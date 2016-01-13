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

status: 
oct 22, 2015: pseudo-code 
Jan 2016: version 1 attempt, with:
	- off-spec:
		- Layer, LayoutLayer DEF namespace shared with main scene - no EXPORT semantics
		- bindables from all layers end up in ProdCon bindables lists, 
			so ViewpointList (for NextViewpoint, PrevViewpoint) will (wrongly) show/allow viewpoints from all Layers 
			(should be just from activeLayer)
	- on-spec:
		- Layer, LayoutLayer pushing and popping its own binding stacks in hyperstack as per specs
		- navigation, menubar work on activeLayer
 */

ivec4 childViewport(ivec4 parentViewport, float *clipBoundary){
	ivec4 vport;
	vport.W = (int)((clipBoundary[1] - clipBoundary[0]) *parentViewport.W);
	vport.X = (int)(parentViewport.X + (clipBoundary[0] * parentViewport.W));
	vport.H = (int)((clipBoundary[3] - clipBoundary[2]) *parentViewport.H);
	vport.Y = (int)(parentViewport.Y + (clipBoundary[2] * parentViewport.H));
	return vport;
}

//Layer has 3 virtual functions for fun/testing, 
//but LayerSet should be the only caller for these 3 normally, according to specs
void prep_Layer(struct X3D_Node * _node){
	Stack *vportstack;
	ivec4 pvport,vport;
	ttglobal tg;
	float *clipBoundary, defaultClipBoundary [] = {0.0f, 1.0f, 0.0f, 1.0f}; // left/right/bottom/top 0,1,0,1

	struct X3D_Layer *node = (struct X3D_Layer*)_node;
	tg = gglobal();



	//push layer.viewport onto viewport stack, setting it as the current window
	vportstack = (Stack *)tg->Mainloop._vportstack;
	pvport = stack_top(ivec4,vportstack); //parent context viewport
	clipBoundary = defaultClipBoundary;
	if(node->viewport)
		clipBoundary = ((struct X3D_Viewport*)(node->viewport))->clipBoundary.p;
	vport = childViewport(pvport,clipBoundary);
	pushviewport(vportstack, vport);

}
void child_Layer(struct X3D_Node * _node){
	Stack *vportstack;
	ttglobal tg;
	struct X3D_Layer *node;
	ttrenderstate rs;

	rs = renderstate();
	node = (struct X3D_Layer*)_node;
	tg = gglobal();
	vportstack = (Stack *)tg->Mainloop._vportstack;
	if(currentviewportvisible(vportstack)){
		setcurrentviewport(vportstack);
		if (rs->render_geom == VF_Geom)
			glClear(GL_DEPTH_BUFFER_BIT); //if another layer has already drawn, don't clear it, just its depth fingerprint
		normalChildren(node->children);
	}
}
void fin_Layer(struct X3D_Node * _node){
	Stack *vportstack;
	ttglobal tg;
	struct X3D_Layer *node = (struct X3D_Layer*)_node;
	tg = gglobal();

	vportstack = (Stack *)tg->Mainloop._vportstack;
	popviewport(vportstack);
	setcurrentviewport(vportstack);




}


struct X3D_Node*  getRayHit(void);
void child_LayerSet(struct X3D_Node * node){
	// has similar responsibilities to render_heir except just for Layer, LayoutLayer children
	// child is the only virtual function for LayerSet
	// Bindables in Core:
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/core.html#BindableChildrenNodes
	// "If there is no LayerSet node defined, there shall be only one set of binding stacks"
	// -that means its up to LayerSet to switch binding stacks, and manage per-layer modelview matrix stack

	if(node && node->_nodeType == NODE_LayerSet){
		int ii,i,j,activeLayer,layerId;
		ttglobal tg;
		struct X3D_LayerSet * layerset;
		ttrenderstate rs;

		rs = renderstate();
		layerset = (struct X3D_LayerSet *)node;
		tg = gglobal();
		activeLayer = layerset->activeLayer;
		if(0) for(i=0;i<layerset->layers.n;i++){
			struct X3D_Layer * layer;
			layer = (struct X3D_Layer*)layerset->layers.p[i];
			prep_Layer((struct X3D_Node*)layer);
			child_Layer((struct X3D_Node*)layer);
			fin_Layer((struct X3D_Node*)layer);

		}
		for(i=0;i<layerset->order.n;i++){

			int i0, saveActive;
			struct X3D_Node *rayhit;
			struct X3D_Layer * layer;
			bindablestack* bstack;

			ii = i;
			if(rs->render_sensitive == VF_Sensitive){
				ii = layerset->order.n - ii -1; //reverse order compared to rendering
			}

			layerId = layerset->order.p[i];
			i0 = layerId -1;
			layer = (struct X3D_Layer*)layerset->layers.p[i0];

			if(rs->render_sensitive == VF_Sensitive){
				if(!layer->isPickable) continue; //skip unpickable layers on sensitive pass
			}

			//push/set binding stacks
			bstack = getBindableStacksByLayer(tg, layerId );
			if(bstack == NULL){
				bstack = malloc(sizeof(bindablestack));
				init_bindablestack(bstack, layerId);
				addBindableStack(tg,bstack);
			}
			//push_bindingstacks(node);
			saveActive = tg->Bindable.activeLayer;
			tg->Bindable.activeLayer = layerId;

			//both layer and layoutlayer can be in here
			if(layer->_nodeType == NODE_Layer){
				prep_Layer((struct X3D_Node*)layer);
				child_Layer((struct X3D_Node*)layer);
				fin_Layer((struct X3D_Node*)layer);
			}
			else if(layer->_nodeType == NODE_LayoutLayer){
				prep_LayoutLayer((struct X3D_Node*)layer);
				child_LayoutLayer((struct X3D_Node*)layer);
				fin_LayoutLayer((struct X3D_Node*)layer);
			}
			rayhit = NULL;
			if(rs->render_sensitive)
				rayhit = getRayHit(); //if there's a clear pick of something on a higher layer, no need to check lower layers
			
			//pop binding stacks
			tg->Bindable.activeLayer = saveActive;

			if(rayhit) break;
		}
		tg->Bindable.activeLayer =  layerset->activeLayer;
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
