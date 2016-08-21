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
#include "Viewer.h"


/*
 http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/layering.html
layer 			- called from layerset on its render and rendray
layerset 		-kindof group, but layers not children: render and rendray
viewport 		-usage 1: info node for Layer/LayoutLayer
				-usage 2: (standalone Group-like) prep (push&set clip), fin(pop), ChildC

status: 
oct 22, 2015: pseudo-code 
Jan 2016: version 1 attempt, with:
	- off-spec:
		- Layer, LayoutLayer DEF namespace/executionContext shared with main scene - no EXPORT semantics (off spec, but handy)
		- bindables from all layers end up in ProdCon bindables lists, 
			so ViewpointList (for NextViewpoint, PrevViewpoint) will (wrongly) show/allow viewpoints from all Layers 
			(should be just from activeLayer)
			- have an extra field in all bindables for node->_layerId if that helps
	- on-spec:
		- Layer, LayoutLayer pushing and popping its own binding stacks in hyperstack as per specs
		- navigation, menubar work on activeLayer
		- per-layer viewer = Viewer()
 */


typedef struct pComponent_Layering{
	int layerId, saveActive, binding_stack_set;
	struct X3D_Node *layersetnode;
}* ppComponent_Layering;
void *Component_Layering_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Layering));
	memset(v,0,sizeof(struct pComponent_Layering));
	return v;
}
void Component_Layering_init(struct tComponent_Layering *t){
	//public
	//private
	t->prv = Component_Layering_constructor();
	{
		ppComponent_Layering p = (ppComponent_Layering)t->prv;
		p->layersetnode = NULL;
		p->layerId = 0;
		p->saveActive = 0;
		p->binding_stack_set = 0;
	}
}
void Component_Layering_clear(struct tComponent_Text *t){
	//public
	//private
	{
		// ppComponent_Layering p = (ppComponent_Layering)t->prv;
		//FREE_IF_NZ(p->xxx);
	}
}

void getPickrayXY(int *x, int *y);
int pointinsideviewport(ivec4 vp, ivec2 pt);
ivec4 childViewport(ivec4 parentViewport, float *clipBoundary){
	ivec4 vport;
	vport.W = (int)((clipBoundary[1] - clipBoundary[0]) *parentViewport.W);
	vport.X = (int)(parentViewport.X + (clipBoundary[0] * parentViewport.W));
	vport.H = (int)((clipBoundary[3] - clipBoundary[2]) *parentViewport.H);
	vport.Y = (int)(parentViewport.Y + (clipBoundary[2] * parentViewport.H));
	return vport;
}
void prep_Viewport(struct X3D_Node * node);
void fin_Viewport(struct X3D_Node * node);
static float defaultClipBoundary [] = {0.0f, 1.0f, 0.0f, 1.0f}; // left/right/bottom/top 0,1,0,1
//Layer has 3 virtual functions prep, children, fin 
//- LayerSet should be the only caller for these 3 normally, according to specs
//- if no LayerSet but a Layer then Layer doesn't do any per-layer stacks or viewer
void prep_Layer(struct X3D_Node * _node){
	ttrenderstate rs;
	struct X3D_Layer *node = (struct X3D_Layer*)_node;

	
	rs = renderstate();

	//There's no concept of window or viewpoint on the 
	// fwl_rendersceneupdatescene(dtime) > render_pre() > render_hier VF_Viewpoint or VF_Collision
	// pass which is just for updating the world and avatar position within the world
	if(!rs->render_vp && !rs->render_collision){
		//push layer.viewport onto viewport stack, setting it as the current window
		if(node->viewport) prep_Viewport(node->viewport);
	}

}
void child_Layer(struct X3D_Node * _node){
	int ivpvis;
	Stack *vportstack;
	ttglobal tg;
	struct X3D_Layer *node;
	ttrenderstate rs;

	rs = renderstate();
	ivpvis = TRUE;
	node = (struct X3D_Layer*)_node;
	if(!rs->render_vp && !rs->render_collision){

		tg = gglobal();
		vportstack = (Stack *)tg->Mainloop._vportstack;
		ivpvis = currentviewportvisible(vportstack);
		if(ivpvis)
			setcurrentviewport(vportstack);
	}
	if(ivpvis){
		if (rs->render_geom == VF_Geom)
			glClear(GL_DEPTH_BUFFER_BIT); //if another layer has already drawn, don't clear it, just its depth fingerprint
		prep_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);
		normalChildren(node->children);
		fin_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);
	}
}
void fin_Layer(struct X3D_Node * _node){
	ttrenderstate rs;
	struct X3D_Layer *node = (struct X3D_Layer*)_node;

	rs = renderstate();

	if(!rs->render_vp && !rs->render_collision){
		if(node->viewport) fin_Viewport(node->viewport);
	}

}


struct X3D_Node*  getRayHit(void);
/*
 How bindables -viewpoint, navigationInfo, background ...- get bound 
 in the correct layer's binding stacks:
 1) during parsing we figure out their layerId, and assign it to
    the bindable's new _layerId field
 2) during set_bind we look at the bindable nodes's _layerId 
    and put it in the binding stacks for that layer
 How we do #1 figure out layerId during parsing:
 a) if no layerSet, everything goes into default layerId 0 (main scene layer)
 b) when we hit a LayerSet during parsing we push_binding_stack_set()
	with starts a list from 0
 c) when we hit a Layer / LayoutLayer during parsing, we add it to the pushed list
	via push_next_layerId_fro_binding_stack_set(), and it sets tg->Bindable.activeLayer
 d) when we parse a bindable, during creation we set its _layerId = tg->bindable.activeLayer
*/
//static int layerId, saveActive, binding_stack_set;
//in theory the 3 parsing temps could be stored in the LayerSet node
void push_binding_stack_set(struct X3D_Node* layersetnode){
	//used during parsing to control layerId for controlling binding stack use
	ttglobal tg = gglobal();
	ppComponent_Layering p = (ppComponent_Layering)tg->Component_Layering.prv;
	p->binding_stack_set++;
	p->saveActive = tg->Bindable.activeLayer;
	p->layerId = 0;
	p->layersetnode = layersetnode; //specs: max one of them per scenefile
}
void push_next_layerId_from_binding_stack_set(struct X3D_Node *layer){
	bindablestack* bstack;
	ttglobal tg = gglobal();
	ppComponent_Layering p = (ppComponent_Layering)tg->Component_Layering.prv;

	//only change binding stacks if there was a LayerSet node otherwise accorindg to specs everything is in one binding stack set (default layerId = 0)
	if(p->binding_stack_set > 0){
		p->layerId ++;
		bstack = getBindableStacksByLayer(tg, p->layerId );
		if(bstack == NULL){
			bstack = MALLOCV(sizeof(bindablestack));
			init_bindablestack(bstack, p->layerId, layer->_nodeType);
			addBindableStack(tg,bstack);
		}
		//push_bindingstacks(node);
		tg->Bindable.activeLayer = p->layerId;
	}
}

void pop_binding_stack_set(){
	ttglobal tg = gglobal();
	ppComponent_Layering p = (ppComponent_Layering)tg->Component_Layering.prv;

	p->binding_stack_set--;
	tg->Bindable.activeLayer = p->saveActive;
}
void post_parse_set_activeLayer(){
	ttglobal tg = gglobal();
	ppComponent_Layering p = (ppComponent_Layering)tg->Component_Layering.prv;
	
	if(p->layersetnode && p->layersetnode->_nodeType == NODE_LayerSet){
		struct X3D_LayerSet *ls = (struct X3D_LayerSet*)p->layersetnode;
		tg->Bindable.activeLayer = ls->activeLayer;
	}
}



void prep_LayoutLayer(struct X3D_Node * _node);
void child_LayoutLayer(struct X3D_Node * _node);
void fin_LayoutLayer(struct X3D_Node * _node);

void setup_viewpoint_part1();
void setup_viewpoint_part3();
void set_viewmatrix();
void setup_projection();
void setup_projection_tinkering();
void upd_ray();
void push_ray();
void pop_ray();
void setup_pickray0();
void child_LayerSet(struct X3D_Node * node){
	// has similar responsibilities to render_heir except just for Layer, LayoutLayer children
	// child is the only virtual function for LayerSet
	// Bindables in Core:
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/core.html#BindableChildrenNodes
	// "If there is no LayerSet node defined, there shall be only one set of binding stacks"
	// -that means its up to LayerSet to switch binding stacks, and manage per-layer modelview matrix stack
	// Picking reverses the order of layers so top layer can 'swallow the mouse'

	if(node && node->_nodeType == NODE_LayerSet){
		int ii,i,layerId;

		// UNUSED OLDCODE activeLayer 

		ttglobal tg;
		struct X3D_LayerSet * layerset;
		ttrenderstate rs;

		rs = renderstate();
		layerset = (struct X3D_LayerSet *)node;
		tg = gglobal();
		// UNUSED OLDCODE activeLayer = layerset->activeLayer;

		for(i=0;i<layerset->order.n;i++){

			int i0, saveActive, isActive;
			struct X3D_Node *rayhit;
			struct X3D_Layer * layer;
			// UNUSED OLDCODE X3D_Viewer *viewer;
			bindablestack* bstack;

			ii = i;
			//if(0) //uncomment to pick in same layer order as render, for diagnostic testing
			if(rs->render_sensitive == VF_Sensitive){
				ii = layerset->order.n - ii -1; //reverse order compared to rendering
			}

			layerId = layerset->order.p[ii];
			isActive = layerId == tg->Bindable.activeLayer;
			i0 = layerId -1;
			layer = (struct X3D_Layer*)layerset->layers.p[i0];

			if(rs->render_sensitive == VF_Sensitive){
				if(!layer->isPickable) continue; //skip unpickable layers on sensitive pass
			}
			if(rs->render_collision == VF_Collision && !isActive)
				continue; //skip non-navigation layers on collision pass


			//push/set binding stacks (some of this done in x3d parsing now, so bstack shouldn't be null)
			bstack = getBindableStacksByLayer(tg, layerId );
			if(bstack == NULL){
				bstack = MALLOCV(sizeof(bindablestack));
				init_bindablestack(bstack, layerId, layer->_nodeType);
				addBindableStack(tg,bstack);
			}
			saveActive = tg->Bindable.activeLayer;
			// UNUSED OLDCODE viewer = (X3D_Viewer*)bstack->viewer;
			// UNUSED OLDCODE if(viewer) printf("layerid=%d ortho=%d ofield=%f %f %f %f\n",layerId,viewer->ortho,viewer->orthoField[0],viewer->orthoField[1],viewer->orthoField[2],viewer->orthoField[3]);

			tg->Bindable.activeLayer = layerId;

			//per-layer modelview matrix is handled here in LayerSet because according
			//to the specs if there is no LayerSet, then there's only one 
			//set of binding stacks (one modelview matrix)

			//push modelview matrix
			if(!isActive){
				FW_GL_MATRIX_MODE(GL_PROJECTION);
				FW_GL_PUSH_MATRIX();
				FW_GL_MATRIX_MODE(GL_MODELVIEW);
				FW_GL_PUSH_MATRIX();
				if(rs->render_vp == VF_Viewpoint){
					setup_viewpoint_part1();
				}else{
					set_viewmatrix();
				}
			}
			if(!rs->render_vp && !rs->render_collision )
				setup_projection();
			if(rs->render_sensitive == VF_Sensitive){
				push_ray();
				if(!isActive) upd_ray();
			}


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
			if(rs->render_sensitive == VF_Sensitive){
				rayhit = getRayHit(); //if there's a clear pick of something on a higher layer, no need to check lower layers
				pop_ray();
			}
			

			//pop modelview matrix
			if(!isActive){
				if(rs->render_vp == VF_Viewpoint){
					setup_viewpoint_part3();
				}
				FW_GL_MATRIX_MODE(GL_PROJECTION);
				FW_GL_POP_MATRIX();
				FW_GL_MATRIX_MODE(GL_MODELVIEW);
				FW_GL_POP_MATRIX();
			}

			//pop binding stacks
			tg->Bindable.activeLayer = saveActive;
			//setup_projection();
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
void prep_Viewport(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		Stack *vportstack;
		ivec4 pvport,vport;
		float *clipBoundary; // left/right/bottom/top 0,1,0,1
		ttrenderstate rs;
		ttglobal tg;
		struct X3D_Viewport * viewport = (struct X3D_Viewport *)node;
		tg = gglobal();

		rs = renderstate();
		//There's no concept of window or viewpoint on the 
		// fwl_rendersceneupdatescene(dtime) > render_pre() > render_hier VF_Viewpoint or VF_Collision
		// pass which is just for updating the world and avatar position within the world
		if(!rs->render_vp && !rs->render_collision){

			//push viewport onto viewport stack, setting it as the current window
			vportstack = (Stack *)tg->Mainloop._vportstack;
			pvport = stack_top(ivec4,vportstack); //parent context viewport
			clipBoundary = defaultClipBoundary;
			if(viewport->clipBoundary.p && viewport->clipBoundary.n > 3)
				clipBoundary = viewport->clipBoundary.p;

			vport = childViewport(pvport,clipBoundary);
			if(rs->render_sensitive){
				int mouseX, mouseY, inside;
				ivec2 pt;
				getPickrayXY(&mouseX, &mouseY);
				pt.X = mouseX;
				pt.Y = mouseY;
				inside = pointinsideviewport(vport,pt);
				if(!inside){
					vport.W = 0;
					vport.H = 0;
				}
			}
			pushviewport(vportstack, vport);
			if(currentviewportvisible(vportstack)){
				setcurrentviewport(vportstack);
			upd_ray();
			}
		}
	}

}

void child_Viewport(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		Stack *vportstack;
		struct X3D_Viewport * viewport;
		ttglobal tg;
		tg = gglobal();

		viewport = (struct X3D_Viewport *)node;
		vportstack = (Stack *)tg->Mainloop._vportstack;

		if(currentviewportvisible(vportstack)){
			prep_sibAffectors((struct X3D_Node*)node,&viewport->__sibAffectors);
			normalChildren(viewport->children);
			fin_sibAffectors((struct X3D_Node*)node,&viewport->__sibAffectors);
		}
	}
}
void fin_Viewport(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Viewport){
		Stack *vportstack;
		ttrenderstate rs;
		ttglobal tg;
		// OLDCODE UNUSED struct X3D_Viewport * viewport = (struct X3D_Viewport *)node;

		// compiler warning mitigation
		UNUSED(rs);

		tg = gglobal();

		rs = renderstate();

		if(!rs->render_vp && !rs->render_collision){

			vportstack = (Stack *)tg->Mainloop._vportstack;

			//pop viewport
			popviewport(vportstack);
			setcurrentviewport(vportstack);
			upd_ray();
		}
	}
}
