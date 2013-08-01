/*
=INSERT_TEMPLATE_HERE=

$Id$

X3D Rendering Component

*/


/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2013 John Alexander Stewart

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
#include "../opengl/Frustum.h"
#include "../opengl/Material.h"
#include "../opengl/OpenGL_Utils.h"
#include "Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"
#include "../scenegraph/Polyrep.h"
#include "Children.h"


/************************************************************************/
/*									*/
/*			CADFace						*/
/*									*/
/************************************************************************/

void child_CADFace (struct X3D_CADFace *node) {
	if (node->shape != NULL) render_node(node->shape);
}

/************************************************************************/
/*									*/
/*			CADAssembly					*/
/*									*/
/************************************************************************/

/* prep_CADAssembly - we need this so that distance (and, thus, distance sorting) works for CADAssembly */
/* refer to prep_Group for detailed explanations */
void prep_CADAssembly (struct X3D_CADAssembly *node) {
    COMPILE_IF_REQUIRED
    RECORD_DISTANCE

}

/*child_CADAssembly - check with child_Group for detailed explanations */
void child_CADAssembly (struct X3D_CADAssembly *node) {
    CHILDREN_COUNT
    LOCAL_LIGHT_SAVE
    
    RETURN_FROM_CHILD_IF_NOT_FOR_ME
    
    /* do we have a DirectionalLight for a child? */
    LOCAL_LIGHT_CHILDREN(node->_sortedChildren);
    
    normalChildren(node->_sortedChildren);
    
    LOCAL_LIGHT_OFF
}

/* we compile the CADAssembly so that children are not continuously sorted */
void compile_CADAssembly (struct X3D_CADAssembly *node) {
    REINITIALIZE_SORTED_NODES_FIELD(node->children,node->_sortedChildren);
    /*
     {
     int i;
     ConsoleMessage ("compile_CADAssembly, rootNode is %p",rootNode());
     for (i=0; i<node->children.n; i++) ConsoleMessage ("compile_CADAssembly %p, c %d is %p",node,i,node->children.p[i]);
     for (i=0; i<node->_sortedChildren.n; i++) ConsoleMessage ("compile_CADAssembly %p, sc %d is %p",node,i,node->_sortedChildren.p[i]);
     }
     */
    MARK_NODE_COMPILED

}

/************************************************************************/
/*									*/
/*			CADLayer					*/
/*									*/
/************************************************************************/


/* render nodes. If visible is < children, just render the children (according to spec 32.4.3) */

void child_CADLayer (struct X3D_CADLayer *node) {
    int i;
    for (i=0; i<node->children.n; i++) {
	if (i >= node->visible.n) render_node(node->children.p[i]); 
        else if (node->visible.p[i]) render_node(node->children.p[i]);
    }
}

/************************************************************************/
/*									*/
/*			CADPart						*/
/*									*/
/************************************************************************/

void prep_CADPart (struct X3D_CADPart *node) {
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


void child_CADPart (struct X3D_CADPart *node) {
	LOCAL_LIGHT_SAVE
	CHILDREN_COUNT
	OCCLUSIONTEST

	RETURN_FROM_CHILD_IF_NOT_FOR_ME

	/* any children at all? */
	if (nc==0) return;

	/* do we have a local light for a child? */
	LOCAL_LIGHT_CHILDREN(node->_sortedChildren);

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

void compile_CADPart (struct X3D_CADPart *node) {
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

void fin_CADPart (struct X3D_CADPart *node) {
	OCCLUSIONTEST

        if(!renderstate()->render_vp) {
            if (node->__do_anything) {
                FW_GL_POP_MATRIX();

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
}


/************************************************************************/
/*									*/
/*			IndexedQuadSet					*/
/*									*/
/************************************************************************/


void render_IndexedQuadSet (struct X3D_IndexedQuadSet *node) {
                COMPILE_POLY_IF_REQUIRED( node->coord, node->color, node->normal, node->texCoord)
		CULL_FACE(node->solid)
		render_polyrep(node);
}

/************************************************************************/
/*									*/
/*			QuadSet						*/
/*									*/
/*									*/
/************************************************************************/

void render_QuadSet (struct X3D_QuadSet *node) {
                COMPILE_POLY_IF_REQUIRED(node->coord, node->color, node->normal, node->texCoord)
		CULL_FACE(node->solid)
		render_polyrep(node);
}

