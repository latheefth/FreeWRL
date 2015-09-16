/*


X3D Grouping Component

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
#include "../vrml_parser/CRoutes.h"
#include "../main/headers.h"

#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Frustum.h"
#include "../opengl/Material.h"

#include "LinearAlgebra.h"
#include "Children.h"

void compile_Transform (struct X3D_Transform *node) { 
	INITIALIZE_EXTENT;

	/* printf ("changed Transform for node %u\n",node); */
	node->__do_center = verify_translate ((GLfloat *)node->center.c);
	node->__do_trans = verify_translate ((GLfloat *)node->translation.c);
	node->__do_scale = verify_scale ((GLfloat *)node->scale.c);
	node->__do_rotation = verify_rotate ((GLfloat *)node->rotation.c);
	node->__do_scaleO = verify_rotate ((GLfloat *)node->scaleOrientation.c);

	node->__do_anything = (node->__do_center ||
			node->__do_trans ||
			node->__do_scale ||
			node->__do_rotation ||
			node->__do_scaleO);

	REINITIALIZE_SORTED_NODES_FIELD(node->children,node->_sortedChildren);
	MARK_NODE_COMPILED
}


/* we compile the Group so that children are not continuously sorted */
void compile_Group(struct X3D_Group *node) {
	REINITIALIZE_SORTED_NODES_FIELD(node->children,node->_sortedChildren);
	/*
	{
		int i;
		ConsoleMessage ("compile_Group, rootNode is %p",rootNode());
		for (i=0; i<node->children.n; i++) ConsoleMessage ("compile_Group %p, c %d is %p",node,i,node->children.p[i]);
		for (i=0; i<node->_sortedChildren.n; i++) ConsoleMessage ("compile_Group %p, sc %d is %p",node,i,node->_sortedChildren.p[i]);
	}
	*/
	MARK_NODE_COMPILED
}

/* prep_Group - we need this so that distance (and, thus, distance sorting) works for Groups */
void prep_Group (struct X3D_Group *node) {
	COMPILE_IF_REQUIRED
	RECORD_DISTANCE

/* 
printf ("prepGroup %p (root %p), flags %x children %d ",node,rootNode,node->_renderFlags,node->children.n);
if ((node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) printf ("VF_Viewpoint ");
if ((node->_renderFlags & VF_Geom) == VF_Geom) printf ("VF_Geom ");
if ((node->_renderFlags & VF_localLight) == VF_localLight) printf ("VF_localLight ");
if ((node->_renderFlags & VF_Sensitive) == VF_Sensitive) printf ("VF_Sensitive ");
if ((node->_renderFlags & VF_Blend) == VF_Blend) printf ("VF_Blend ");
if ((node->_renderFlags & VF_Proximity) == VF_Proximity) printf ("VF_Proximity ");
if ((node->_renderFlags & VF_Collision) == VF_Collision) printf ("VF_Collision ");
if ((node->_renderFlags & VF_globalLight) == VF_globalLight) printf ("VF_globalLight ");
if ((node->_renderFlags & VF_hasVisibleChildren) == VF_hasVisibleChildren) printf ("VF_hasVisibleChildren ");
if ((node->_renderFlags & VF_shouldSortChildren) == VF_shouldSortChildren) printf ("VF_shouldSortChildren ");*/
/*if ((node->_renderFlags & VF_inPickableGroup) == VF_inPickableGroup) printf ("VF_inPickableGroup "); */
/* printf ("\n"); */



}

/* do transforms, calculate the distance */
void prep_Transform (struct X3D_Transform *node) {

	COMPILE_IF_REQUIRED

        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	/* printf ("prep_Transform, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

	/* do we have any geometry visible, and are we doing anything with geometry? */
	OCCLUSIONTEST

	if(!renderstate()->render_vp) {
		/* do we actually have any thing to rotate/translate/scale?? */
		if (node->__do_anything) {

		FW_GL_PUSH_MATRIX();

		/* TRANSLATION */
		if (node->__do_trans)
			FW_GL_TRANSLATE_F(node->translation.c[0],node->translation.c[1],node->translation.c[2]);

		/* CENTER */
		if (node->__do_center)
			FW_GL_TRANSLATE_F(node->center.c[0],node->center.c[1],node->center.c[2]);

		/* ROTATION */
		if (node->__do_rotation) {
			FW_GL_ROTATE_RADIANS(node->rotation.c[3], node->rotation.c[0],node->rotation.c[1],node->rotation.c[2]);
		}

		/* SCALEORIENTATION */
		if (node->__do_scaleO) {
			FW_GL_ROTATE_RADIANS(node->scaleOrientation.c[3], node->scaleOrientation.c[0], node->scaleOrientation.c[1],node->scaleOrientation.c[2]);
		}


		/* SCALE */
		if (node->__do_scale)
			FW_GL_SCALE_F(node->scale.c[0],node->scale.c[1],node->scale.c[2]);

		/* REVERSE SCALE ORIENTATION */
		if (node->__do_scaleO)
			FW_GL_ROTATE_RADIANS(-node->scaleOrientation.c[3], node->scaleOrientation.c[0], node->scaleOrientation.c[1],node->scaleOrientation.c[2]);

		/* REVERSE CENTER */
		if (node->__do_center)
			FW_GL_TRANSLATE_F(-node->center.c[0],-node->center.c[1],-node->center.c[2]);
		} 

		RECORD_DISTANCE

        }
}


void fin_Transform (struct X3D_Transform *node) {
	OCCLUSIONTEST

        if(!renderstate()->render_vp) {
            if (node->__do_anything) {
		FW_GL_POP_MATRIX();
	    }
        } else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if((node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) {
                FW_GL_TRANSLATE_F(((node->center).c[0]),((node->center).c[1]),((node->center).c[2])
                );
                FW_GL_ROTATE_RADIANS(((node->scaleOrientation).c[3]),((node->scaleOrientation).c[0]),((node->scaleOrientation).c[1]),((node->scaleOrientation).c[2])
                );
                FW_GL_SCALE_F((float)1.0/(((node->scale).c[0])),(float)1.0/(((node->scale).c[1])),(float)1.0/(((node->scale).c[2]))
                );
                FW_GL_ROTATE_RADIANS(-(((node->scaleOrientation).c[3])),((node->scaleOrientation).c[0]),((node->scaleOrientation).c[1]),((node->scaleOrientation).c[2])
                );
                FW_GL_ROTATE_RADIANS(-(((node->rotation).c[3])),((node->rotation).c[0]),((node->rotation).c[1]),((node->rotation).c[2])
                );
                FW_GL_TRANSLATE_F(-(((node->center).c[0])),-(((node->center).c[1])),-(((node->center).c[2]))
                );
                FW_GL_TRANSLATE_F(-(((node->translation).c[0])),-(((node->translation).c[1])),-(((node->translation).c[2]))
                );
            }
        }
} 

void child_Switch (struct X3D_Switch *node) {
	/* exceedingly simple - render only one child */
	int wc = node->whichChoice;

	/* is this VRML, or X3D?? */
	if (node->__isX3D) {
		if(wc >= 0 && wc < ((node->children).n)) {
			void *p = ((node->children).p[wc]);
			render_node(p);
		}
	} else {
		if(wc >= 0 && wc < ((node->choice).n)) {
			void *p = ((node->choice).p[wc]);
			render_node(p);
		}
	}
}


void child_StaticGroup (struct X3D_StaticGroup *node) {
	CHILDREN_COUNT
	LOCAL_LIGHT_SAVE

	RETURN_FROM_CHILD_IF_NOT_FOR_ME

	/* did this change? */
	if NODE_NEEDS_COMPILING {
		REINITIALIZE_SORTED_NODES_FIELD(node->children,node->_sortedChildren);
		//ConsoleMessage ("StaticGroup changed");
		MARK_NODE_COMPILED;
	}

	/* do we have a local light for a child? */
	LOCAL_LIGHT_CHILDREN(node->_sortedChildren);

	/* now, just render the non-directionalLight children */
	normalChildren(node->_sortedChildren);

	LOCAL_LIGHT_OFF
}

void child_Group (struct X3D_Group *node) {
	int renderFirstProtoChildOnlyAsPerSpecs = 1;
	CHILDREN_COUNT
	LOCAL_LIGHT_SAVE

	/*
printf ("chldGroup %p (root %p), flags %x children %d ",node,rootNode,node->_renderFlags,node->children.n);
if ((node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) printf ("VF_Viewpoint ");
if ((node->_renderFlags & VF_Geom) == VF_Geom) printf ("VF_Geom ");
if ((node->_renderFlags & VF_localLight) == VF_localLight) printf ("VF_localLight ");
if ((node->_renderFlags & VF_Sensitive) == VF_Sensitive) printf ("VF_Sensitive ");
if ((node->_renderFlags & VF_Blend) == VF_Blend) printf ("VF_Blend ");
if ((node->_renderFlags & VF_Proximity) == VF_Proximity) printf ("VF_Proximity ");
if ((node->_renderFlags & VF_Collision) == VF_Collision) printf ("VF_Collision ");
if ((node->_renderFlags & VF_globalLight) == VF_globalLight) printf ("VF_globalLight ");
if ((node->_renderFlags & VF_hasVisibleChildren) == VF_hasVisibleChildren) printf ("VF_hasVisibleChildren ");
if ((node->_renderFlags & VF_shouldSortChildren) == VF_shouldSortChildren) printf ("VF_shouldSortChildren ");
#ifdef DJTRACK_PICKSENSORS
if ((node->_renderFlags & VF_inPickableGroup) == VF_inPickableGroup) printf ("VF_inPickableGroup ");
#endif
printf ("\n");
*/

	RETURN_FROM_CHILD_IF_NOT_FOR_ME

	if(1){
		//stereoscopic experiments
		ttrenderstate rs = renderstate();
		if (rs->render_geom) { //== VF_Geom) {
			if (node->_renderFlags & VF_HideLeft && (viewer_iside() == 0) )  { 
					return; 
			} 
			if (node->_renderFlags & VF_HideRight && (viewer_iside() == 1) )  { 
					return; 
			} 
		} 
	}


#ifdef VERBOSE
	 {
		int x;
		struct X3D_Node *xx;

printf ("child_Group,  children.n %d sortedChildren.n %d\n",node->children.n, node->_sortedChildren.n);

		printf ("child_Group, this %p rf %x isProto %d\n",node,node->_renderFlags, node->FreeWRL__protoDef);
//        printf ("	..., render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
//         render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); 
		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->_sortedChildren.p[x]);
			if (xx)
			printf ("	%d: ch %p type %s dist %f\n",x, node->_sortedChildren.p[x],stringNodeType(xx->_nodeType),xx->_dist);
			else printf ("     chiuld %d null\n",x);
		}
		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->_sortedChildren.p[x]);
			if (xx)
			printf ("	%d: sch %p type %s dist %f\n",x, node->_sortedChildren.p[x],stringNodeType(xx->_nodeType),xx->_dist);
			else printf ("     chiuld %d null\n",x);
		}
	}
#endif //VERBOSE




		
	/* do we have a DirectionalLight for a child? */
	LOCAL_LIGHT_CHILDREN(node->_sortedChildren);

	/* printf ("chld_Group, for %u, protodef %d and FreeWRL_PROTOInterfaceNodes.n %d\n",
		node, node->FreeWRL__protoDef, node->FreeWRL_PROTOInterfaceNodes.n); */
	/* now, just render the non-directionalLight children */
	renderFirstProtoChildOnlyAsPerSpecs = 0; //flux/vivaty render all children
	if ((node->FreeWRL__protoDef!=INT_ID_UNDEFINED) && renderstate()->render_geom 
		&& renderFirstProtoChildOnlyAsPerSpecs) {
		(node->children).n = 1;
		normalChildren(node->children);
		(node->children).n = nc;
	} else {
		normalChildren(node->_sortedChildren);
	}

	LOCAL_LIGHT_OFF
}


void child_Transform (struct X3D_Transform *node) {
	LOCAL_LIGHT_SAVE
	CHILDREN_COUNT
	OCCLUSIONTEST

	RETURN_FROM_CHILD_IF_NOT_FOR_ME

	if(1){
		//stereoscopic experiments
		ttrenderstate rs = renderstate();
		if (rs->render_geom) { //== VF_Geom) {
			if (node->_renderFlags & VF_HideLeft && (viewer_iside() == 0) )  { 
					return; 
			} 
			if (node->_renderFlags & VF_HideRight && (viewer_iside() == 1) )  { 
					return; 
			} 
		} 
	}

	/* any children at all? */
	if (nc==0) return;

	//profile_start("local_light_kids");
	/* do we have a local light for a child? */
	LOCAL_LIGHT_CHILDREN(node->_sortedChildren);
	//profile_end("local_light_kids");
	/* now, just render the non-directionalLight children */

	/* printf ("Transform %d, flags %d, render_sensitive %d\n",
			node,node->_renderFlags,render_sensitive); */

	#ifdef CHILDVERBOSE
		printf ("transform - doing normalChildren\n");
	#endif

	normalChildren(node->_sortedChildren);

	#ifdef CHILDVERBOSE
		printf ("transform - done normalChildren\n");
	#endif

	LOCAL_LIGHT_OFF
}


/* prep_Proto - this is a ProtoInstance (not declare)  */
/*pack 4 flags into one int, using char */
char ciflag_get(int flags, int index){
	char *cflags = (char *)(&flags);
	return cflags[index];
}
int ciflag_set(int flags, char flag, int index ){
	char *cflags = (char *)(&flags);
	cflags[index] = flag;
	return flags;
}
void prep_Proto (struct X3D_Proto *node) {
	if(0)printf("in prep_proto\n");
	load_externProtoInstance(node);
	COMPILE_IF_REQUIRED
	//RECORD_DISTANCE
}
/* not sure why we would compile */
void compile_Proto(struct X3D_Proto *node) {
	unsigned char pflag;
	if(0)printf("in compile_proto\n");
	pflag = ciflag_get(node->__protoFlags,2);
	if(pflag == 2){
		//scene
		REINITIALIZE_SORTED_NODES_FIELD(node->__children,node->_sortedChildren);
	}
	MARK_NODE_COMPILED
}
/* render the first node only */
void child_Proto (struct X3D_Proto *node) {
	int nc;
	unsigned char sceneflag;
	int renderFirstProtoChildOnlyAsPerSpecs;
	LOCAL_LIGHT_SAVE
	if(0)printf("in child_proto\n");
	//CHILDREN_COUNT
	nc = node->__children.n; //_sortedChildren.n;
/*
printf ("chldGroup %p (root %p), flags %x children %d ",node,rootNode,node->_renderFlags,node->children.n);
if ((node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) printf ("VF_Viewpoint ");
if ((node->_renderFlags & VF_Geom) == VF_Geom) printf ("VF_Geom ");
if ((node->_renderFlags & VF_localLight) == VF_localLight) printf ("VF_localLight ");
if ((node->_renderFlags & VF_Sensitive) == VF_Sensitive) printf ("VF_Sensitive ");
if ((node->_renderFlags & VF_Blend) == VF_Blend) printf ("VF_Blend ");
if ((node->_renderFlags & VF_Proximity) == VF_Proximity) printf ("VF_Proximity ");
if ((node->_renderFlags & VF_Collision) == VF_Collision) printf ("VF_Collision ");
if ((node->_renderFlags & VF_globalLight) == VF_globalLight) printf ("VF_globalLight ");
if ((node->_renderFlags & VF_hasVisibleChildren) == VF_hasVisibleChildren) printf ("VF_hasVisibleChildren ");
if ((node->_renderFlags & VF_shouldSortChildren) == VF_shouldSortChildren) printf ("VF_shouldSortChildren ");
#ifdef DJTRACK_PICKSENSORS
if ((node->_renderFlags & VF_inPickableGroup) == VF_inPickableGroup) printf ("VF_inPickableGroup ");
#endif
printf ("\n");
*/
	RETURN_FROM_CHILD_IF_NOT_FOR_ME
	//if(node->__loadstatus != LOAD_STABLE) return; #define LOAD_STABLE 10



#ifdef VERBOSE
	 {
		int x;
		struct X3D_Node *xx;

printf ("child_Group,  children.n %d sortedChildren.n %d\n",node->children.n, node->_sortedChildren.n);

		printf ("child_Group, this %p rf %x isProto %d\n",node,node->_renderFlags, node->FreeWRL__protoDef);
//        printf ("	..., render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
//         render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); 
		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->_sortedChildren.p[x]);
			if (xx)
			printf ("	%d: ch %p type %s dist %f\n",x, node->_sortedChildren.p[x],stringNodeType(xx->_nodeType),xx->_dist);
			else printf ("     chiuld %d null\n",x);
		}
		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->_sortedChildren.p[x]);
			if (xx)
			printf ("	%d: sch %p type %s dist %f\n",x, node->_sortedChildren.p[x],stringNodeType(xx->_nodeType),xx->_dist);
			else printf ("     chiuld %d null\n",x);
		}
	}
#endif //VERBOSE




		
	/* do we have a DirectionalLight for a child? */
	if(nc){
		LOCAL_LIGHT_CHILDREN(node->__children);
	}else{
		LOCAL_LIGHT_CHILDREN(node->_sortedChildren);
	}

	/* printf ("chld_Group, for %u, protodef %d and FreeWRL_PROTOInterfaceNodes.n %d\n",
		node, node->FreeWRL__protoDef, node->FreeWRL_PROTOInterfaceNodes.n); */
	/* now, just render the non-directionalLight children */
	//if ((node->FreeWRL__protoDef!=INT_ID_UNDEFINED) && renderstate()->render_geom) {
	//	(node->children).n = 1;
	//	normalChildren(node->children);
	//	(node->children).n = nc;
	//} else {
	//	normalChildren(node->_sortedChildren);
	//}
	sceneflag = ciflag_get(node->__protoFlags,2);
	renderFirstProtoChildOnlyAsPerSpecs = FALSE;
	//I don't think inline.children comes through here, just scene and protoInstance
	if(sceneflag == 2 ){ 
		normalChildren(node->_sortedChildren);
	}else{
		if(renderFirstProtoChildOnlyAsPerSpecs && renderstate()->render_geom) {
			(node->__children).n = 1;
			normalChildren(node->__children);
			(node->__children).n = nc;
		} else {
			normalChildren(node->__children);
		}
	}

	LOCAL_LIGHT_OFF

}
