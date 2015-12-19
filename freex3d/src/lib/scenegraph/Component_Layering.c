/*


X3D Layering Component

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

#include "../x3d_parser/Bindable.h"
#include "Children.h"
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/RenderFuncs.h"


// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/layering.html

ivec4 childViewport(ivec4 parentViewport, float *clipBoundary){
	ivec4 vport;
	vport.W = (int)((clipBoundary[1] - clipBoundary[0]) *parentViewport.W);
	vport.X = (int)(parentViewport.X + (clipBoundary[0] * parentViewport.W));
	vport.H = (int)((clipBoundary[3] - clipBoundary[2]) *parentViewport.H);
	vport.Y = (int)(parentViewport.Y + (clipBoundary[2] * parentViewport.H));
	return vport;
}

//status: pseudo-code oct 22, 2015
void render_LayerSet(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_LayerSet){
		int i,j;
		ttglobal tg;
		
		struct X3D_LayerSet * layerset = (struct X3D_LayerSet *)node;
		tg = gglobal();
		for(i=0;i<layerset->layers.n;i++){
			struct X3D_Layer * layer;
			Stack *vportstack;
			ivec4 pvport,vport;
			float *clipBoundary, defaultClipBoundary [] = {0.0f, 1.0f, 0.0f, 1.0f}; // left/right/bottom/top 0,1,0,1

			layerset->activeLayer = j = layerset->order.p[i];
			layer = (struct X3D_Layer*)layerset->layers.p[j];
			//push/set binding stacks
			//push layer.viewport onto viewport stack, setting it as the current window
			vportstack = (Stack *)tg->Mainloop._vportstack;
			pvport = stack_top(ivec4,vportstack); //parent context viewport
			clipBoundary = defaultClipBoundary;
			if(layer->viewport)
				clipBoundary = ((struct X3D_Viewport*)(layer->viewport))->clipBoundary.p;
			vport = childViewport(pvport,clipBoundary);
			pushviewport(vportstack, vport);
			if(currentviewportvisible(vportstack)){
				setcurrentviewport(vportstack);
				glClear(GL_DEPTH_BUFFER_BIT); //if another layer has already drawn, don't clear it, just its depth fingerprint
				render_node((struct X3D_Node*)layer);
			}
			popviewport(vportstack);
			setcurrentviewport(vportstack);
			//pop binding stacks
		}
	}
}

//I suspect I don't need a rendray_ rather just a children handler that can transform a pickray before calling normalchildren
void rendray_LayerSet(struct X3D_Node *node){
	//for picking
	//float h,r,y;
	struct point_XYZ t_r1,t_r2;
	get_current_ray(&t_r1, &t_r2);

}
void render_Layer(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Layer){
		struct X3D_Layer * layer = (struct X3D_Layer *)node;
		normalChildren(layer->children);
	}
}
// maybe don't need rendray_layer, just a children handler that transforms pickray before calling normalchildren
void rendray_Layer(struct X3D_Node *node){
	//for picking
	//float h,r,y;
	struct point_XYZ t_r1,t_r2;

	get_current_ray(&t_r1, &t_r2);

}
//not sure what I need for viewport. 
//Situation #1 standalone viewport:
//Maybe there should be a push and pop from a viewport stack, if rendering its children
// ie pre: push vport
// render: render children via normalChildren
// fin/post: pop vport
//Situation #2 viewport in SFNode viewport field of another layer / layout node
// the host node would do
// pre: push vport
// render: render itself 
// post/fin: pop vport
void render_Viewport(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		struct X3D_Viewport * viewport = (struct X3D_Viewport *)node;
		//push viewport
		normalChildren(viewport->children);
		//pop viewport
	}
}
//not sure how to do this, except we need to transform the pickray
// for example a pickray may start over whole screen, then be transformed into
// one quad of a quadrant display for further picking
void rendray_Viewport(struct X3D_Node *node){
	//for picking
	//float h,r,y;
	struct point_XYZ t_r1,t_r2;

	get_current_ray(&t_r1, &t_r2);
	//push viewport
	//transform pickray to viewport and push pickray
	//rendray children
	//pop pickray
	//pop viewport

}