/*


Bindable nodes - Background, TextureBackground, Fog, NavigationInfo, Viewpoint, GeoViewpoint.

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CRoutes.h"
#include "../opengl/OpenGL_Utils.h"
#include "Bindable.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../scenegraph/Component_Geospatial.h"
#include "../scenegraph/RenderFuncs.h"
#include "../scenegraph/Component_ProgrammableShaders.h"
#include "../scenegraph/Component_Shape.h"
#include "../ui/common.h"
#include "../scenegraph/LinearAlgebra.h"

/* for Background spheres */
struct MyVertex
 {
   struct SFVec3f vert;    //Vertex
   struct SFColorRGBA col;     //Colour
 };



static void saveBGVert (float *colptr, float *pt, int *vertexno, float *col, double dist, double x, double y, double z) ;

void Bindable_init(struct tBindable *t){
	//public
	t->naviinfo.width = 0.25;
	t->naviinfo.height = 1.6;
	t->naviinfo.step = 0.75;

	t->background_stack = newVector(struct X3D_Node*, 2);
	t->viewpoint_stack = newVector(struct X3D_Node*, 2);
	t->fog_stack = newVector(struct X3D_Node*, 2);
	t->navigation_stack = newVector(struct X3D_Node*, 2);
}
void Bindable_clear(struct tBindable *t){
	//public
	 deleteVector(struct X3D_Node*, t->background_stack);
	 deleteVector(struct X3D_Node*, t->viewpoint_stack);
	 deleteVector(struct X3D_Node*, t->fog_stack);
	 deleteVector(struct X3D_Node*, t->navigation_stack);
}
/* common entry routine for setting avatar size */
void set_naviWidthHeightStep(double wid, double hei, double step) {
	ttglobal tg = gglobal();
	tg->Bindable.naviinfo.width = wid;
	tg->Bindable.naviinfo.height = hei;
	tg->Bindable.naviinfo.step = step;

	/* printf ("set_naviWdithHeightStep - width %lf height %lf step %lf speed %lf\n",wid,hei,step,Viewer.speed); */

}

/* called when binding NavigationInfo nodes */
void set_naviinfo(struct X3D_NavigationInfo *node) {
	struct Uni_String **svptr;
	int i;
	char *typeptr;
	X3D_Viewer *viewer = Viewer();

        viewer->speed = (double) node->speed;
	if (node->avatarSize.n<2) {
		printf ("set_naviinfo, avatarSize smaller than expected\n");
	} else {
		set_naviWidthHeightStep ((double)(node->avatarSize.p[0]),
			(double)(node->avatarSize.p[1]),
			(double)((node->avatarSize.p[2]))); //dug9 Jan 6, 2010 - this is too crazy for the new gravity. * node->speed) * 2));
	}

	/* keep track of valid Navigation types. */
	svptr = node->type.p;

	/* assume "NONE" is set */
	for (i=0; i<16; i++) viewer->oktypes[i] = FALSE;


	/* now, find the ones that are ok */
	for (i = 0; i < node->type.n; i++) {
		/*  get the string pointer */
		typeptr = svptr[i]->strptr;

		if (strcmp(typeptr,"WALK") == 0) {
			viewer->oktypes[VIEWER_WALK] = TRUE;
			if (i==0) fwl_set_viewer_type(VIEWER_WALK);
		}
		if (strcmp(typeptr,"FLY") == 0) {
			viewer->oktypes[VIEWER_FLY] = TRUE;
			if (i==0) fwl_set_viewer_type(VIEWER_FLY);
		}
		if (strcmp(typeptr,"EXAMINE") == 0) {
			viewer->oktypes[VIEWER_EXAMINE] = TRUE;
			if (i==0) fwl_set_viewer_type(VIEWER_EXAMINE);
		}
		if (strcmp(typeptr,"NONE") == 0) {
			viewer->oktypes[VIEWER_NONE] = TRUE;
			if (i==0) fwl_set_viewer_type(VIEWER_NONE);
		}
		if (strcmp(typeptr,"EXFLY") == 0) {
			viewer->oktypes[VIEWER_EXFLY] = TRUE;
			if (i==0) fwl_set_viewer_type(VIEWER_EXFLY);
		}
		if (strcmp(typeptr,"EXPLORE") == 0) {
			viewer->oktypes[VIEWER_EXPLORE] = TRUE;
			if (i==0) fwl_set_viewer_type(VIEWER_EXPLORE);
		}
		if (strcmp(typeptr,"LOOKAT") == 0) {
			viewer->oktypes[VIEWER_LOOKAT] = TRUE;
			//if (i==0) fwl_set_viewer_type(VIEWER_LOOKAT);
		}
		if (strcmp(typeptr,"SPHERICAL") == 0) {
			viewer->oktypes[VIEWER_SPHERICAL] = TRUE;
			if (i==0) fwl_set_viewer_type(VIEWER_SPHERICAL);
		}
		if (strcmp(typeptr, "TURNTABLE") == 0) {
			viewer->oktypes[VIEWER_TURNTABLE] = TRUE;
			if (i == 0) fwl_set_viewer_type(VIEWER_TURNTABLE);
		}
		if (strcmp(typeptr, "ANY") == 0) {
			viewer->oktypes[VIEWER_EXAMINE] = TRUE;
			viewer->oktypes[VIEWER_WALK] = TRUE;
			viewer->oktypes[VIEWER_EXFLY] = TRUE;
			viewer->oktypes[VIEWER_FLY] = TRUE;
			viewer->oktypes[VIEWER_EXPLORE] = TRUE;
			viewer->oktypes[VIEWER_LOOKAT] = TRUE;
			viewer->oktypes[VIEWER_SPHERICAL] = TRUE;
			viewer->oktypes[VIEWER_TURNTABLE] = TRUE;
			if (i==0) fwl_set_viewer_type (VIEWER_WALK); /*  just choose one */
		}
	}
        viewer->headlight = node->headlight;
	/* tell the menu buttons of the state of this headlight */
	//setMenuButton_headlight(node->headlight);

	/* transition effects */
	viewer->transitionTime = node->transitionTime;
	/* bounds checking */
	if (viewer->transitionTime < 0.0) viewer->transitionTime = 0.0;

	viewer->transitionType = VIEWER_TRANSITION_LINEAR; /* assume LINEAR */
	if (node->transitionType.n > 0) {
		if (strcmp("LINEAR", node->transitionType.p[0]->strptr) == 0) viewer->transitionType = VIEWER_TRANSITION_LINEAR;
		else if (strcmp("TELEPORT", node->transitionType.p[0]->strptr) == 0) viewer->transitionType = VIEWER_TRANSITION_TELEPORT;
		else if (strcmp("ANIMATE", node->transitionType.p[0]->strptr) == 0) viewer->transitionType = VIEWER_TRANSITION_ANIMATE;
		else {
			ConsoleMessage ("Unknown NavigationInfo transitionType :%s:",node->transitionType.p[0]->strptr);
		}
	}

}




/* send a set_bind event from an event to this Bindable node */
void send_bind_to(struct X3D_Node *node, int value) {
	ttglobal tg = gglobal();
	/* printf ("\n%lf: send_bind_to, nodetype %s node %u value %d\n",TickTime(),stringNodeType(node->_nodeType),node,value);  */

	switch (node->_nodeType) {

	case NODE_Background:  {
		struct X3D_Background *bg = (struct X3D_Background *) node;
		bg->set_bind = value;
		bind_node (node, tg->Bindable.background_stack);
		break;
		}

	case NODE_TextureBackground: {
		struct X3D_TextureBackground *tbg = (struct X3D_TextureBackground *) node;
		tbg->set_bind = value;
		bind_node (node, tg->Bindable.background_stack);
		break;
		}

	case NODE_OrthoViewpoint: {
		struct X3D_OrthoViewpoint *ovp = (struct X3D_OrthoViewpoint *) node;
		ovp->set_bind = value;
		setMenuStatusVP(ovp->description->strptr);
		bind_node (node, tg->Bindable.viewpoint_stack);
		if (value==1) {
			bind_OrthoViewpoint (ovp);
		}
		break;
		}

	case NODE_Viewpoint:  {
		struct X3D_Viewpoint* vp = (struct X3D_Viewpoint *) node;
		vp->set_bind = value;
		setMenuStatusVP (vp->description->strptr);
		bind_node (node, tg->Bindable.viewpoint_stack);
		if (value==1) {
			bind_Viewpoint (vp);
		}
		break;
		}

	case NODE_GeoViewpoint:  {
		struct X3D_GeoViewpoint *gvp = (struct X3D_GeoViewpoint *) node;
		gvp->set_bind = value;
		setMenuStatusVP (gvp->description->strptr);
		bind_node (node, tg->Bindable.viewpoint_stack);
		if (value==1) {
			bind_GeoViewpoint (gvp);
		}
		break;
		}


	case NODE_Fog:  {
		struct X3D_Fog *fg = (struct X3D_Fog *) node;
		fg->set_bind = value;
		bind_node (node, tg->Bindable.fog_stack);
		break;
		}

	case NODE_NavigationInfo:  {
		struct X3D_NavigationInfo *nv = (struct X3D_NavigationInfo *) node;
		nv->set_bind = value;
		bind_node (node, tg->Bindable.navigation_stack);
		if (value==1) set_naviinfo(nv);
		break;
		}

	default:
		ConsoleMessage("send_bind_to, cant send a set_bind to %s %p!!\n",stringNodeType(node->_nodeType),node);
	}
}




/* Do binding for node and stack - works for all bindable nodes */

/* return the setBind offset of this node */
static size_t setBindofst(void *node) {
	struct X3D_Background *tn;
	tn = (struct X3D_Background *) node;
	switch (tn->_nodeType) {
		case NODE_Background: return offsetof(struct X3D_Background, set_bind);
		case NODE_TextureBackground: return offsetof(struct X3D_TextureBackground, set_bind);
		case NODE_Viewpoint: return offsetof(struct X3D_Viewpoint, set_bind);
		case NODE_OrthoViewpoint: return offsetof(struct X3D_OrthoViewpoint, set_bind);
		case NODE_GeoViewpoint: return offsetof(struct X3D_GeoViewpoint, set_bind);
		case NODE_Fog: return offsetof(struct X3D_Fog, set_bind);
		case NODE_NavigationInfo: return offsetof(struct X3D_NavigationInfo, set_bind);
		default: {printf ("setBindoffst - huh? node type %d\n",tn->_nodeType); }
	}
	return 0;
}

/* return the isBound offset of this node */
static size_t bindTimeoffst (struct X3D_Node  *node) {
	X3D_NODE_CHECK(node);

	switch (node->_nodeType) {
		case NODE_Background: return offsetof(struct X3D_Background, bindTime);
		case NODE_TextureBackground: return offsetof(struct X3D_TextureBackground, bindTime);
		case NODE_Viewpoint: return offsetof(struct X3D_Viewpoint, bindTime);
		case NODE_OrthoViewpoint: return offsetof(struct X3D_Viewpoint, bindTime);
		case NODE_GeoViewpoint: return offsetof(struct X3D_GeoViewpoint, bindTime);
		case NODE_Fog: return offsetof(struct X3D_Fog, bindTime);
		case NODE_NavigationInfo: return offsetof(struct X3D_NavigationInfo, bindTime);
		default: {printf ("bindTimeoffst  - huh? node type %s\n",stringNodeType(node->_nodeType)); }
	}
	return 0;
}

/* return the isBound offset of this node */
static size_t isboundofst(void *node) {
	struct X3D_Background *tn;

	/* initialization */
	tn = (struct X3D_Background *) node;

	X3D_NODE_CHECK(node);

	switch (tn->_nodeType) {
		case NODE_Background: return offsetof(struct X3D_Background, isBound);
		case NODE_TextureBackground: return offsetof(struct X3D_TextureBackground, isBound);
		case NODE_Viewpoint: return offsetof(struct X3D_Viewpoint, isBound);
		case NODE_OrthoViewpoint: return offsetof(struct X3D_Viewpoint, isBound);
		case NODE_GeoViewpoint: return offsetof(struct X3D_GeoViewpoint, isBound);
		case NODE_Fog: return offsetof(struct X3D_Fog, isBound);
		case NODE_NavigationInfo: return offsetof(struct X3D_NavigationInfo, isBound);
		default: {printf ("isBoundoffst - huh? node type %s\n",stringNodeType(tn->_nodeType)); }
	}
	return 0;
}
int removeNodeFromVector(int iaction, struct Vector *v, struct X3D_Node *node);
void bind_node (struct X3D_Node *node, struct Vector *thisStack) {
	int *isBoundPtr;
	int *setBindPtr;
	size_t offst;
 
	isBoundPtr = offsetPointer_deref(int*, node, isboundofst(node));
	setBindPtr = offsetPointer_deref(int*, node, setBindofst(node));
	
 	#ifdef BINDVERBOSE
 	printf ("bind_node, node %p (%s), set_bind %d isBound %d\n",node,stringNodeType(node->_nodeType),*setBindPtr,*isBoundPtr);
 	#endif

	/* is this guy already bound? */
	if (*isBoundPtr && (*setBindPtr != 0) ){
		#ifdef BINDVERBOSE
		printf("%p already bound\n",node);
		#endif
		*setBindPtr = 100;
	return; } /* It has to be at the top of the stack so return */
 

	 /* we either have a setBind of 1, which is a push, or 0, which
	    is a pop. the value of 100 (arbitrary) indicates that this
	    is not a new push or pop */

	/* is this a push? */
	if (*setBindPtr == 1) {
		/* PUSH THIS TO THE TOP OF THE STACK */

		/* isBound mimics setBind */
		*isBoundPtr = 1;

		/* unset the set_bind flag  - setBind can be 0 or 1; lets make it garbage */
		*setBindPtr = 100;

		MARK_EVENT (node, (unsigned int) isboundofst(node));

		/* set up the "bindTime" field */
		offst = bindTimeoffst(node);
		if (offst != 0) {
			double *dp;
			dp = offsetPointer_deref(double*, node, offst);
			*dp = TickTime();
			MARK_EVENT (node, offst);
		}

		/* unbind the one below, unless it is same node */
		if (vectorSize(thisStack)>0) {
			struct X3D_Node* oldTOS;

			oldTOS = vector_back(struct X3D_Node *,thisStack);
			/* printf ("already have a node here...have to unbind it %p %p\n",node,oldTOS); */

			if (oldTOS == node) return;  /* do not unbind */
			isBoundPtr = offsetPointer_deref(int*, oldTOS, isboundofst(oldTOS));
			setBindPtr = offsetPointer_deref(int*, oldTOS, setBindofst(oldTOS));
			*isBoundPtr = 0;
			*setBindPtr = 100;
			MARK_EVENT (oldTOS, (unsigned int) isboundofst(oldTOS));
		}

		/* push it now */
		vector_pushBack(struct X3D_Node*,thisStack,node);


	} else if (*setBindPtr == 0) {
		/* POP FROM TOP OF STACK  - if we ARE the top of stack */
		/* isBound mimics setBind */
		*isBoundPtr = 0;

		/* unset the set_bind flag  - setBind can be 0 or 1; lets make it garbage */
		*setBindPtr = 100;

		MARK_EVENT (node, (unsigned int) isboundofst(node));

		/* are we top of stack? */
		if (vectorSize(thisStack)>0) {
			struct X3D_Node* oldTOS;

			oldTOS = vector_back(struct X3D_Node *,thisStack);
			/* printf ("already have a node here...have to unbind it %p %p\n",node,oldTOS); */

			if (oldTOS != node) { 
				if(!removeNodeFromVector(0, thisStack, node)){
					if (node->_nodeType == NODE_Viewpoint){
						printf ("can not pop from stack, not top (%p != %p)\n",node,oldTOS);
						printf ("%p Viewpoint, description :%s:\n",node,X3D_VIEWPOINT(node)->description->strptr);
						printf ("%p Viewpoint, description :%s:\n",oldTOS,X3D_VIEWPOINT(oldTOS)->description->strptr);
						printf ("oldTOS, isBound %d, setBindPtr %d\n",*(offsetPointer_deref(int*, oldTOS, isboundofst(oldTOS))), 
						*(offsetPointer_deref(int*, oldTOS, setBindofst(oldTOS))));
						printf("and not found in stack\n");
					}
				}
				return;
			} else {
				/* we are top of stack... */
				/* get myself off of the stack */
//if (node->_nodeType == NODE_Viewpoint) {
//int j;
//printf ("%p Viewpoint, description :%s:\n",node,X3D_VIEWPOINT(node)->description->strptr);
//printf("stacksize before popping=%d ",vectorSize(thisStack));
//for(j=0;j<vectorSize(thisStack);j++){
//struct X3D_Viewpoint *vp = vector_get(struct X3D_Viewpoint *,thisStack,j);
//printf ("index= %d %p Viewpoint, description :%s:\n",j,node,vp->description->strptr);
//}
//}
				vector_popBack(struct X3D_Node *,thisStack);
//if (node->_nodeType == NODE_Viewpoint) {
//printf("stacksize after popping=%d ",vectorSize(thisStack));
//				if(removeNodeFromVector(0, thisStack, node)){
//					printf("but still found and removed from stack\n");
//				}else{
//					printf("and now not found in stack\n");
//				}
//
//}
				removeNodeFromVector(0, thisStack, node); //sometimes there are duplicates further down the stack. for unloading inlines, we need to get rid of all occurrances
				if (vectorSize(thisStack)>0) {
					/* get the older one back */
					oldTOS = vector_back(struct X3D_Node *,thisStack);

					/* set it to be bound */
					isBoundPtr = offsetPointer_deref(int*, oldTOS, isboundofst(oldTOS));
					setBindPtr = offsetPointer_deref(int*, oldTOS, setBindofst(oldTOS));
					*isBoundPtr = 1;
					*setBindPtr = 100;
					MARK_EVENT (oldTOS, (unsigned int) isboundofst(oldTOS));
				}
			} 
		} else {
			/* printf ("stack is zero size, can not pop off\n"); */
		}
		

	} else {
		printf ("setBindPtr %d\n",*setBindPtr);
	}
}


void render_Fog (struct X3D_Fog *node) {
	#ifndef GL_ES_VERSION_2_0 /* this should be handled in material shader */
	GLDOUBLE mod[16];
	GLDOUBLE proj[16];
	GLDOUBLE x,y,z;
	GLDOUBLE x1,y1,z1;
	GLDOUBLE sx, sy, sz;
	GLfloat fog_colour [4];
	char *fogptr;
	int foglen;
	GLDOUBLE unit[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	ttglobal tg = gglobal();

	UNUSED(foglen); //mitigate compiler warnings - should eventually use this variable though!

	/* printf ("render_Fog, node %d isBound %d color %f %f %f set_bind %d\n",
	node, node->isBound, node->color.c[0],node->color.c[1],node->color.c[2],node->set_bind); */

	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {

		bind_node (X3D_NODE(node), tg->Bindable.fog_stack);

		/* if we do not have any more nodes on top of stack, disable fog */
		glDisable(GL_FOG);
	}

	if(!node->isBound) return;
	if (node->visibilityRange <= 0.00001) return;

	fog_colour[0] = node->color.c[0];
	fog_colour[1] = node->color.c[1];
	fog_colour[2] = node->color.c[2];
	fog_colour[3] = (float) 1.0;

	fogptr = node->fogType->strptr;
	foglen = node->fogType->len;
	FW_GL_PUSH_MATRIX();
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, mod);
	FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, proj);
	/* Get origin */
	FW_GLU_UNPROJECT(0.0f,0.0f,0.0f,mod,proj,viewport,&x,&y,&z);
	FW_GL_TRANSLATE_D(x,y,z);

	FW_GLU_UNPROJECT(0.0f,0.0f,0.0f,mod,unit,viewport,&x,&y,&z);
	/* Get scale */
	FW_GLU_PROJECT(x+1,y,z,mod,unit,viewport,&x1,&y1,&z1);
	sx = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	FW_GLU_PROJECT(x,y+1,z,mod,unit,viewport,&x1,&y1,&z1);
	sy = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	FW_GLU_PROJECT(x,y,z+1,mod,unit,viewport,&x1,&y1,&z1);
	sz = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
	/* Undo the translation and scale effects */
	FW_GL_SCALE_D(sx,sy,sz);


	/* now do the foggy stuff */
	FW_GL_FOGFV(GL_FOG_COLOR,fog_colour);

	/* make the fog look like the examples in the VRML Source Book */
	if (strcmp("LINEAR",fogptr)) {
		/* Exponential */
		FW_GL_FOGF(GL_FOG_DENSITY, (float) (4.0)/ (node->visibilityRange));
		FW_GL_FOGF(GL_FOG_END, (float) (node->visibilityRange));
		FW_GL_FOGI(GL_FOG_MODE, GL_EXP);
	} else {
		/* Linear */
		FW_GL_FOGF(GL_FOG_START, (float) 1.0);
		FW_GL_FOGF(GL_FOG_END, (float) (node->visibilityRange));
		FW_GL_FOGI(GL_FOG_MODE, GL_LINEAR);
	}
	glEnable(GL_FOG);

	FW_GL_POP_MATRIX();
	#endif /* GL_ES_VERSION_2_0 this should be handled in material shader */
}


/******************************************************************************
 *
 * Background, TextureBackground stuff 
 *
 ******************************************************************************/

/* save a Background vertex into the __points and __colours arrays */
static void saveBGVert (float *colptr, float *pt,
		int *vertexno, float *col, double dist,
		double x, double y, double z) {
		/* save the colour */
		memcpy (&colptr[*vertexno*3], col, sizeof(float)*3);

		/* and, save the vertex info */
		pt[*vertexno*3+0] = (float)(x*dist);
		pt[*vertexno*3+1] = (float)(y*dist);
		pt[*vertexno*3+2] = (float)(z*dist);

		(*vertexno)++;
}

/* the background centre follows our position, so, move it! */
static void moveBackgroundCentre () {
	GLDOUBLE mod[16];
	GLDOUBLE proj[16];
	GLDOUBLE unit[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	GLDOUBLE x,y,z;
	GLDOUBLE x1,y1,z1;
	GLDOUBLE sx, sy, sz;
	ttglobal tg = gglobal();

	FW_GL_PUSH_MATRIX();
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, mod);
	if(0){
		FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, proj);
		/* Get origin */
		FW_GLU_UNPROJECT(0.0f,0.0f,0.0f,mod,proj,viewport,&x,&y,&z);
		FW_GL_TRANSLATE_D(x,y,z);

		LIGHTING_OFF

		FW_GLU_UNPROJECT(0.0f,0.0f,0.0f,mod,unit,viewport,&x,&y,&z);
		/* Get scale */
		FW_GLU_PROJECT(x+1,y,z,mod,unit,viewport,&x1,&y1,&z1);
		sx = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
		FW_GLU_PROJECT(x,y+1,z,mod,unit,viewport,&x1,&y1,&z1);
		sy = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );
		FW_GLU_PROJECT(x,y,z+1,mod,unit,viewport,&x1,&y1,&z1);
		sz = 1/sqrt( x1*x1 + y1*y1 + z1*z1*4 );

		/* Undo the translation and scale effects */
		FW_GL_SCALE_D(sx,sy,sz);
		//printf("moveBackground old T %f %f %f old S %f %f %f\n",x,y,z,sx,sy,sz);
	}
	if(1){
		//feature-AFFINE_GLU_UNPROJECT
		//translation, scale same as glu_unproject (no 4*z needed for this way)
		double modi[16];
		struct point_XYZ p, q;
		p.x = p.y = p.z = 0.0;
		matinverseAFFINE(modi,mod);
		transform(&p,&p,modi);
		FW_GL_TRANSLATE_D(p.x,p.y,p.z);

		LIGHTING_OFF

		/* Get scale */
		q = p;
		q.x += 1.0;
		transform(&q,&q,mod);
		sx = 1.0/sqrt( q.x*q.x + q.y*q.y + q.z*q.z );
		q = p;
		q.y += 1.0;
		transform(&q,&q,mod);
		sy = 1.0/sqrt( q.x*q.x + q.y*q.y + q.z*q.z );
		q = p;
		q.z += 1.0;
		transform(&q,&q,mod);
		sz = 1.0/sqrt( q.x*q.x + q.y*q.y + q.z*q.z );
		/* Undo the translation and scale effects */
		FW_GL_SCALE_D(sx,sy,sz);
		//printf("moveBackground new T %f %f %f new S %f %f %f\n",x,y,z,sx,sy,sz);
		//printf("\n");
	}
}

static void recalculateBackgroundVectors(struct X3D_Background *node) {
	struct SFColor *c1,*c2;
	int hdiv;			/* number of horizontal strips allowed */
	int h,v;
	double va1, va2, ha1, ha2;	/* JS - vert and horiz angles 	*/
	int estq;
	int actq;

	/* filled in if this is a TextureBackground node */
	struct X3D_TextureBackground *tbnode;

	/* generic structures between nodes used for taking individual pointers from node defns */
	struct SFColor *skyCol; int skyColCt;
	struct SFColor *gndCol; int gndColCt;
	float  *skyAng; int skyAngCt;
	float  *gndAng; int gndAngCt;
	float *newPoints; float *newColors;
	double outsideRadius, insideRadius;

	/* initialization */
	tbnode = NULL;
	hdiv = 20;

	/* We draw spheres, one for the sky, one for the ground - outsideRadius and insideRadius */
	outsideRadius =  DEFAULT_FARPLANE* 0.750;
	insideRadius = DEFAULT_FARPLANE * 0.50;

	/* lets try these values - we will scale when we draw this */
	outsideRadius = 1.0;
	insideRadius = 0.5;

	/* handle Background and TextureBackgrounds here */
	if (node->_nodeType == NODE_Background) {
		skyCol = node->skyColor.p;
		gndCol = node ->groundColor.p;
		skyColCt = node->skyColor.n;
		gndColCt = node->groundColor.n;
		skyAng = node->skyAngle.p;
		gndAng = node ->groundAngle.p;
		skyAngCt = node->skyAngle.n;
		gndAngCt = node->groundAngle.n;
	} else {
		tbnode = (struct X3D_TextureBackground *) node;
		skyCol = tbnode->skyColor.p;
		gndCol = tbnode ->groundColor.p;
		skyColCt = tbnode->skyColor.n;
		gndColCt = tbnode->groundColor.n;
		skyAng = tbnode->skyAngle.p;
		gndAng = tbnode ->groundAngle.p;
		skyAngCt = tbnode->skyAngle.n;
		gndAngCt = tbnode->groundAngle.n;
	}

	/* do we have NO background triangles? (ie, maybe all textures??) */
	if ((skyColCt == 0) & (gndColCt == 0)) {
        	if (node->_nodeType == NODE_Background) {
			MARK_NODE_COMPILED
                	/* do we have an old background to destroy? */
                	FREE_IF_NZ (node->__points.p);
                	FREE_IF_NZ (node->__colours.p);
                	node->__quadcount = 0;
        	} else {
                	tbnode->_ichange = tbnode->_change; /* mimic MARK_NODE_COMPILED */

                	/* do we have an old background to destroy? */
                	FREE_IF_NZ (tbnode->__points.p);
                	FREE_IF_NZ (tbnode->__colours.p);
                	tbnode->__quadcount = 0;
        	}
		return;
	}


	/* calculate how many quads are required */
	estq=0; actq=0;
	if(skyColCt == 1) {
		estq += 40;
	} else {
		estq += (skyColCt-1) * 20 + 20;
		/* attempt to find exact estimate, fails if no skyAngle, so
		 simply changed above line to add 20 automatically.
		if ((skyColCt >2) &&
			(skyAngCt > skyColCt-2)) {
			if (skyAng[skyColCt-2] < (PI-0.01))
				estq += 20;
		}
		*/
	}

	if(gndColCt == 1) estq += 40;
	else if (gndColCt>0) estq += (gndColCt-1) * 20;

	/* now, MALLOC space for new arrays  - 3 points per vertex, 6 per quad. */
	newPoints = MALLOC (GLfloat *, sizeof (GLfloat) * estq * 3 * 6);
	newColors = MALLOC (GLfloat *, sizeof (GLfloat) * estq * 3 * 6);


	if(skyColCt == 1) {
		c1 = &skyCol[0];
		va1 = 0;
		va2 = PI/2;

		for(v=0; v<2; v++) {
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;
				/* 0 */ saveBGVert (newColors, newPoints, &actq,&c1->c[0],outsideRadius, sin(va2)*cos(ha1), cos(va2), sin(va2)*sin(ha1));
				/* 1 */ saveBGVert (newColors, newPoints, &actq,&c1->c[0],outsideRadius, sin(va2)*cos(ha2), cos(va2), sin(va2)*sin(ha2));
				/* 2 */ saveBGVert (newColors, newPoints, &actq,&c1->c[0],outsideRadius, sin(va1)*cos(ha2), cos(va1), sin(va1)*sin(ha2));
				/* 0 */ saveBGVert (newColors, newPoints, &actq,&c1->c[0],outsideRadius, sin(va2)*cos(ha1), cos(va2), sin(va2)*sin(ha1));
				/* 2 */ saveBGVert (newColors, newPoints, &actq,&c1->c[0],outsideRadius, sin(va1)*cos(ha2), cos(va1), sin(va1)*sin(ha2));
				/* 3 */ saveBGVert (newColors, newPoints, &actq,&c1->c[0],outsideRadius, sin(va1)*cos(ha1), cos(va1), sin(va1) * sin(ha1));
			}
			va1 = va2;
			va2 = PI;
		}
	} else {
		va1 = 0;
		/* this gets around a compiler warning - we really DO want last values of this from following
		   for loop */
		c1 = &skyCol[0];
		if (skyAngCt>0) {
			va2= skyAng[0];
		} else {
			va2 = PI/2;
		}
		c2=c1;


		for(v=0; v<(skyColCt-1); v++) {
			c1 = &skyCol[v];
			c2 = &skyCol[v+1];
			if (skyAngCt>0) { va2 = skyAng[v];}
			else { va2 = PI/2; }

			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;
				/* 0 */ saveBGVert(newColors,newPoints, &actq,&c2->c[0],outsideRadius, sin(va2)*cos(ha1), cos(va2), sin(va2) * sin(ha1));
				/* 1 */ saveBGVert(newColors,newPoints, &actq,&c2->c[0],outsideRadius, sin(va2)*cos(ha2), cos(va2), sin(va2) * sin(ha2));
				/* 2 */ saveBGVert(newColors,newPoints, &actq,&c1->c[0],outsideRadius, sin(va1)*cos(ha2), cos(va1), sin(va1) * sin(ha2));
				/* 0 */ saveBGVert(newColors,newPoints, &actq,&c2->c[0],outsideRadius, sin(va2)*cos(ha1), cos(va2), sin(va2) * sin(ha1));
				/* 2 */ saveBGVert(newColors,newPoints, &actq,&c1->c[0],outsideRadius, sin(va1)*cos(ha2), cos(va1), sin(va1) * sin(ha2));
				/* 3 */ saveBGVert(newColors,newPoints, &actq,&c1->c[0],outsideRadius, sin(va1) * cos(ha1), cos(va1), sin(va1) * sin(ha1));
			}
			va1 = va2;
		}

		/* now, the spec states: "If the last skyAngle is less than pi, then the
		  colour band between the last skyAngle and the nadir is clamped to the last skyColor." */
		if (va2 < (PI-0.01)) {
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;
				/* 0 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],outsideRadius, sin(PI) * cos(ha1), cos(PI), sin(PI) * sin(ha1));
				/* 1 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],outsideRadius, sin(PI) * cos(ha2), cos(PI), sin(PI) * sin(ha2));
				/* 2 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],outsideRadius, sin(va2) * cos(ha2), cos(va2), sin(va2) * sin(ha2));
				/* 0 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],outsideRadius, sin(PI) * cos(ha1), cos(PI), sin(PI) * sin(ha1));
				/* 2 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],outsideRadius, sin(va2) * cos(ha2), cos(va2), sin(va2) * sin(ha2));
				/* 3 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],outsideRadius, sin(va2) * cos(ha1), cos(va2), sin(va2) * sin(ha1));
			}
		}
	}

	/* Do the ground, if there is anything  to do. */

	if (gndColCt>0) {
		if(gndColCt == 1) {
			c1 = &gndCol[0];
			for(h=0; h<hdiv; h++) {
				ha1 = h * PI*2 / hdiv;
				ha2 = (h+1) * PI*2 / hdiv;

				/* 0 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(PI) * cos(ha1), cos(PI), sin(PI) * sin(ha1));
				/* 1 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(PI) * cos(ha2), cos(PI), sin(PI) * sin(ha2));
				/* 2 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(PI/2) * cos(ha2), cos(PI/2), sin(PI/2) * sin(ha2));
				/* 0 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(PI) * cos(ha1), cos(PI), sin(PI) * sin(ha1));
				/* 2 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(PI/2) * cos(ha2), cos(PI/2), sin(PI/2) * sin(ha2));
				/* 3 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(PI/2) * cos(ha1), cos(PI/2), sin(PI/2) * sin(ha1));
			}
		} else {
			va1 = PI;
			for(v=0; v<gndColCt-1; v++) {
				c1 = &gndCol[v];
				c2 = &gndCol[v+1];
				if (v>=gndAngCt) va2 = PI; /* bounds check */
				else va2 = PI - gndAng[v];

				for(h=0; h<hdiv; h++) {
					ha1 = h * PI*2 / hdiv;
					ha2 = (h+1) * PI*2 / hdiv;

					/* 0 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(va1)*cos(ha1), cos(va1), sin(va1)*sin(ha1));
					/* 1 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(va1)*cos(ha2), cos(va1), sin(va1)*sin(ha2));
					/* 2 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],insideRadius, sin(va2)*cos(ha2), cos(va2), sin(va2)*sin(ha2));
					/* 0 */ saveBGVert(newColors,newPoints,&actq,&c1->c[0],insideRadius, sin(va1)*cos(ha1), cos(va1), sin(va1)*sin(ha1));
					/* 2 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],insideRadius, sin(va2)*cos(ha2), cos(va2), sin(va2)*sin(ha2));
					/* 3 */ saveBGVert(newColors,newPoints,&actq,&c2->c[0],insideRadius, sin(va2) * cos(ha1), cos(va2), sin(va2)*sin(ha1));
				}
				va1 = va2;
			}
		}
	}

	/* We have guessed at the quad count; lets make sure
	 * we record what we have. */
	if (actq > (estq*6)) {
		printf ("Background quadcount error, %d > %d\n",
				actq,estq);
		actq = 0;
	}

	/* save changes */
	/* if we are doing shaders, we write the vertex and color info to a VBO, else we keep pointers in the node */
	if (node->_nodeType == NODE_Background) {

		MARK_NODE_COMPILED

		/* do we have an old background to destroy? */
		FREE_IF_NZ (node->__points.p);
		FREE_IF_NZ (node->__colours.p);
		node->__quadcount = actq;
	} else {
		tbnode->_ichange = tbnode->_change; /* mimic MARK_NODE_COMPILED */
		/* do we have an old background to destroy? */
		FREE_IF_NZ (tbnode->__points.p);
		FREE_IF_NZ (tbnode->__colours.p);
		tbnode->__quadcount = actq;

	}


	{
		struct MyVertex *combinedBuffer = MALLOC(struct MyVertex *, sizeof (struct MyVertex) * actq * 2);
		int i;
		float *npp = newPoints;
		float *ncp = newColors;


		if (node->_nodeType == NODE_Background) {
			if (node->__VBO == 0) glGenBuffers(1,(unsigned int*) &node->__VBO);
		} else {
			if (tbnode->__VBO == 0) glGenBuffers(1,(unsigned int*) &tbnode->__VBO);
		}

		/* stream both the vertex and colours together (could have done this above, but
		   maybe can redo this if we go 100% material shaders */

		/* NOTE - we use SFColorRGBA - and set the Alpha to 1 so that we can use the
		   shader with other nodes with Color fields */

		for (i=0; i<actq; i++) {
			combinedBuffer[i].vert.c[0] = *npp; npp++;
			combinedBuffer[i].vert.c[1] = *npp; npp++;
			combinedBuffer[i].vert.c[2] = *npp; npp++;
			combinedBuffer[i].col.c[0] = *ncp; ncp++;
			combinedBuffer[i].col.c[1] = *ncp; ncp++;
			combinedBuffer[i].col.c[2] = *ncp; ncp++;
			combinedBuffer[i].col.c[3] = 1.0f;
		}
		FREE_IF_NZ(newPoints);
		FREE_IF_NZ(newColors);

		/* send this data along ... */
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,node->__VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof (struct MyVertex)*actq, combinedBuffer, GL_STATIC_DRAW);
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER,0);

		/* and, we can free it */
		FREE_IF_NZ(combinedBuffer);
	}
}

void render_Background (struct X3D_Background *node) {
	ttglobal tg = gglobal();
    
	X3D_Viewer *viewer = Viewer();
	/* if we are rendering blended nodes, don't bother with this one */
	if (renderstate()->render_blend) return;

	/* printf ("RBG, num %d node %d ib %d sb %d gepvp\n",node->__BGNumber, node,node->isBound,node->set_bind);    */
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		bind_node (X3D_NODE(node), tg->Bindable.background_stack);
	}

	/* don't even bother going further if this node is not bound on the top */
	if(!node->isBound) return;

	if (vectorSize(tg->Bindable.fog_stack) >0) glDisable(GL_FOG);

	/* Cannot start_list() because of moving center, so we do our own list later */
	moveBackgroundCentre();

	if (NODE_NEEDS_COMPILING) {
		recalculateBackgroundVectors(node);
	}

	/* we have a sphere (maybe one and a half, as the sky and ground are different) so scale it up so that
	   all geometry fits within the spheres 
		dug9 Sept 2014: background could in theory be a tiny box or sphere that wraps around the avatar, if
		you can draw it first on each frame _and_ turn off 'depth' when you draw it.   
	*/
	FW_GL_SCALE_D (viewer->backgroundPlane, viewer->backgroundPlane, viewer->backgroundPlane);

		enableGlobalShader(getMyShader(COLOUR_MATERIAL_SHADER));

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__VBO);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);

		#define BUFFER_OFFSET(i) ((char *)NULL + (i))
		FW_GL_VERTEX_POINTER(3, GL_FLOAT, (GLsizei) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices
		FW_GL_COLOR_POINTER(4, GL_FLOAT, (GLsizei) sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(sizeof(struct SFVec3f)));   //The starting point of Colours, 12 bytes away

		sendArraysToGPU (GL_TRIANGLES, 0, node->__quadcount);

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);
		finishedWithGlobalShader();

	/* now, for the textures, if they exist */
	if (((node->backUrl).n>0) ||
			((node->frontUrl).n>0) ||
			((node->leftUrl).n>0) ||
			((node->rightUrl).n>0) ||
			((node->topUrl).n>0) ||
			((node->bottomUrl).n>0)) {

        	glEnable(GL_TEXTURE_2D);

        	FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,BackgroundVert);
        	FW_GL_NORMAL_POINTER (GL_FLOAT,0,Backnorms);
        	FW_GL_TEXCOORD_POINTER (2,GL_FLOAT,0,boxtex);

		enableGlobalShader(getMyShader(ONE_TEX_APPEARANCE_SHADER));


		loadBackgroundTextures(node);

		finishedWithGlobalShader();
	}
	FW_GL_POP_MATRIX();

	/* is fog enabled? if so, disable it right now */
	if (vectorSize(tg->Bindable.fog_stack) >0) glEnable(GL_FOG);
}


void render_TextureBackground (struct X3D_TextureBackground *node) {
	ttglobal tg = gglobal();
    
	X3D_Viewer *viewer = Viewer();
	/* if we are rendering blended nodes, don't bother with this one */
	if (renderstate()->render_blend) return;


	/* printf ("RTBG, node %d ib %d sb %d gepvp\n",node,node->isBound,node->set_bind);  */
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		bind_node (X3D_NODE(node), tg->Bindable.background_stack);
	}

	/* don't even bother going further if this node is not bound on the top */
	if(!node->isBound) return;

	/* is fog enabled? if so, disable it right now */
	if (vectorSize(tg->Bindable.fog_stack) >0) glDisable(GL_FOG);

	/* Cannot start_list() because of moving center, so we do our own list later */
	moveBackgroundCentre();

	if  NODE_NEEDS_COMPILING
		/* recalculateBackgroundVectors will determine exact node type */
		recalculateBackgroundVectors((struct X3D_Background *)node);	

	/* we have a sphere (maybe one and a half, as the sky and ground are different) so scale it up so that
	   all geometry fits within the spheres */
	FW_GL_SCALE_D (viewer->backgroundPlane, viewer->backgroundPlane, viewer->backgroundPlane);


		enableGlobalShader(getMyShader(COLOUR_MATERIAL_SHADER));

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, node->__VBO);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);

		#define BUFFER_OFFSET(i) ((char *)NULL + (i))
		FW_GL_VERTEX_POINTER(3, GL_FLOAT, sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(0));   //The starting point of the VBO, for the vertices
		FW_GL_COLOR_POINTER(4, GL_FLOAT, sizeof(struct MyVertex), (GLfloat *)BUFFER_OFFSET(sizeof(struct SFVec3f)));   //The starting point of Colours, 12 bytes away

		sendArraysToGPU (GL_TRIANGLES, 0, node->__quadcount);

		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);
		finishedWithGlobalShader();

	/* now, for the textures, if they exist */
	if ((node->backTexture !=0) ||
			(node->frontTexture !=0) ||
			(node->leftTexture !=0) ||
			(node->rightTexture !=0) ||
			(node->topTexture !=0) ||
			(node->bottomTexture !=0)) {


		enableGlobalShader(getMyShader(ONE_TEX_APPEARANCE_SHADER));



		loadTextureBackgroundTextures(node);

		finishedWithGlobalShader();
	}

	/* pushes are done in moveBackgroundCentre */
	FW_GL_POP_MATRIX();

	if (vectorSize(tg->Bindable.fog_stack) >0) glEnable (GL_FOG);
}
