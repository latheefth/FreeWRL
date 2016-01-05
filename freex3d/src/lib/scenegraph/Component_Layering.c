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


/*
 http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/layering.html
layer 			- called from layerset on its render and rendray
layerset 		-kindof group, but layers not children: render and rendray
viewport 		-use 1: info node 
				-use 2: (standalone Group-like) prep (push&set clip), fin(pop), ChildC

 */

ivec4 childViewport(ivec4 parentViewport, float *clipBoundary){
	ivec4 vport;
	vport.W = (int)((clipBoundary[1] - clipBoundary[0]) *parentViewport.W);
	vport.X = (int)(parentViewport.X + (clipBoundary[0] * parentViewport.W));
	vport.H = (int)((clipBoundary[3] - clipBoundary[2]) *parentViewport.H);
	vport.Y = (int)(parentViewport.Y + (clipBoundary[2] * parentViewport.H));
	return vport;
}

void render_Layer(struct X3D_Node * _node){
	struct X3D_Layer *node = (struct X3D_Layer*)_node;
	normalChildren(node->children);
}
void rendray_Layer(struct X3D_Node * node){
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

			layerset->activeLayer = j = layerset->order.p[i] -1;
			//both layer and layoutlayer can be in here
			//if you want to be able to downcaste layoutlayer to layer, you better have the fields
			//in the same order 
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
				if(layer->_nodeType == NODE_Layer)
					render_Layer((struct X3D_Node*)layer);
				else if(layer->_nodeType == NODE_LayoutLayer)
					render_LayoutLayer((struct X3D_Node*)layer);
			}
			popviewport(vportstack);
			setcurrentviewport(vportstack);
			//pop binding stacks
		}
	}
}
struct X3D_Node*  getRayHit(void);
void rendray_LayerSet(struct X3D_Node * node){
	//picking comes in here, we iterate backward over layers, 
	//starting with the topmost (last drawn) layer
	//until we hit a layer that handles it, then we break 
	if(node && node->_nodeType == NODE_LayerSet){
		int i,j,ii;
		ttglobal tg;
		
		struct X3D_LayerSet * layerset = (struct X3D_LayerSet *)node;
		tg = gglobal();
		for(ii=0;ii<layerset->layers.n;ii++){
			struct X3D_Layer * layer;
			Stack *vportstack;
			ivec4 pvport,vport;
			float *clipBoundary, defaultClipBoundary [] = {0.0f, 1.0f, 0.0f, 1.0f}; // left/right/bottom/top 0,1,0,1

			i = layerset->layers.n - ii -1; //reverse order compared to rendering
			layerset->activeLayer = j = layerset->order.p[i] -1;
			layer = (struct X3D_Layer*)layerset->layers.p[j];
			if(layer->isPickable){
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
					if(layer->_nodeType == NODE_Layer)
						rendray_Layer((struct X3D_Node*)layer);
					else if(layer->_nodeType == NODE_LayoutLayer)
						rendray_LayoutLayer((struct X3D_Node*)layer);
					//if handled, break;
					if(getRayHit()) break;
				}
				popviewport(vportstack);
				setcurrentviewport(vportstack);
				//pop binding stacks
			}
		}
	}
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
void upd_ray();
void prep_Viewport(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		Stack *vportstack;
		ivec4 pvport,vport;
		ttglobal tg;
		struct X3D_Viewport * viewport = (struct X3D_Viewport *)node;
		tg = gglobal();

		//push viewport onto viewport stack, setting it as the current window
		vportstack = (Stack *)tg->Mainloop._vportstack;
		pvport = stack_top(ivec4,vportstack); //parent context viewport

		vport = childViewport(pvport,viewport->clipBoundary.p);
		pushviewport(vportstack, vport);
		if(currentviewportvisible(vportstack))
			setcurrentviewport(vportstack);
		upd_ray();
	}

}
void child_Viewport_old(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		Stack *vportstack;
		ivec4 pvport,vport;
		ttglobal tg;
		struct X3D_Viewport * viewport = (struct X3D_Viewport *)node;
		tg = gglobal();

		//push viewport onto viewport stack, setting it as the current window
		vportstack = (Stack *)tg->Mainloop._vportstack;
		pvport = stack_top(ivec4,vportstack); //parent context viewport

		vport = childViewport(pvport,viewport->clipBoundary.p);
		pushviewport(vportstack, vport);
		if(currentviewportvisible(vportstack)){
			setcurrentviewport(vportstack);

			normalChildren(viewport->children);
		}
		//pop viewport
		popviewport(vportstack);
		setcurrentviewport(vportstack);
	}
}
void child_Viewport(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		struct X3D_Viewport * viewport = (struct X3D_Viewport *)node;
		normalChildren(viewport->children);
	}
}
void fin_Viewport(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		Stack *vportstack;
		ivec4 pvport,vport;
		ttglobal tg;
		struct X3D_Viewport * viewport = (struct X3D_Viewport *)node;
		tg = gglobal();

		vportstack = (Stack *)tg->Mainloop._vportstack;

		//pop viewport
		popviewport(vportstack);
		setcurrentviewport(vportstack);
		upd_ray();
	}
}
