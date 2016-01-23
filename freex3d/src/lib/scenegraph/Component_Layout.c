/*


X3D Layout Component

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

// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/layout.html
/* oct 2015 not sure what I need here
a few easy ones: 
ScreenFontStyle is the same as FontStyle except with pixelSize instead of size
- I suspect implementation code would be primarily in Component_Text.c L.1326
- size (FontStyle) and pointSize (ScreenFontStyle) are both float, so no diff in struct, just NODE_Type
ScreenGroup is like Group or perhaps Transform, except it may need to alter the transform stack 
	with x,y scales so as to treat children's coords as being in screen pixels.
Layout			- (just info node used in other nodes)
LayoutGroup		- (like Group, sets viewport, layout) prep, fin, ChildC [compileC]
LayoutLayer		- always called from layerset: render, rendray
ScreenFontStyle	- (same as FontStyle node - just info node for Text)
ScreenGroup		- (like Group except modifies scale) prep, fin, ChildC [compileC]
(note regular Group is funny: it sorts children by depth often, for transparency blending purposes
	- but I assume we won't do any sorting by depth from our group-like nodes, therefore we don't need Compile)
*/
enum {
	LAYOUT_LEFT,
	LAYOUT_CENTER,
	LAYOUT_RIGHT,
	LAYOUT_BOTTOM,
	LAYOUT_TOP,
	LAYOUT_NONE,
	LAYOUT_WORLD,
	LAYOUT_FRACTION,
	LAYOUT_PIXEL,
	LAYOUT_STRETCH,
};

typedef struct layout_scale_item {
	float scale[2];
	int scalemode[2];
} layout_scale_item;
static layout_scale_item default_layout_scale_item = {1.0f,1.0f,LAYOUT_FRACTION,LAYOUT_FRACTION};

typedef struct pComponent_Layout{
	Stack *layout_scale_stack;
}* ppComponent_Layout;
void *Component_Layout_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Layout));
	memset(v,0,sizeof(struct pComponent_Layout));
	return v;
}
void Component_Layout_init(struct tComponent_Layout *t){
	//public
	//private
	t->prv = Component_Layout_constructor();
	{
		ppComponent_Layout p = (ppComponent_Layout)t->prv;
		p->layout_scale_stack = newVector(layout_scale_item, 2);
		stack_push(layout_scale_item,p->layout_scale_stack,default_layout_scale_item);
	}
}
void Component_Layout_clear(struct tComponent_Text *t){
	//public
	//private
	{
		ppComponent_Layout p = (ppComponent_Layout)t->prv;
		//FREE_IF_NZ(p->xxx);
		deleteStack(layout_scale_item,p->layout_scale_stack);
	}
}



#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif
struct layoutmode {
	char *key;
	int type;
} layoutmodes [] = {
	{"LEFT",LAYOUT_LEFT},
	{"CENTER",LAYOUT_CENTER},
	{"RIGHT",LAYOUT_RIGHT},
	{"BOTTOM",LAYOUT_BOTTOM},
	{"TOP",LAYOUT_TOP},
	{"NONE",LAYOUT_NONE},
	{"WORLD",LAYOUT_WORLD},
	{"FRACTION",LAYOUT_FRACTION},
	{"PIXEL",LAYOUT_PIXEL},
	{"STRETCH",LAYOUT_STRETCH},
	{NULL,0},
};
int lookup_layoutmode(char *cmode){
	int i;
	int retval;
	struct layoutmode *lm;
	i = 0;
	retval = 0;
	do{
		lm = &layoutmodes[i];
		if(!strcasecmp(lm->key,cmode)){
			retval = lm->type;
			break;
		}
		i++;
	}while(layoutmodes[i].key);
	return retval;
}
void prep_Layout(struct X3D_Node *_node);
void fin_Layout(struct X3D_Node *_node);
void getPickrayXY(int *x, int *y);
void prep_Viewport(struct X3D_Node * node);
void fin_Viewport(struct X3D_Node * node);
int pointinsideviewport(ivec4 vp, ivec2 pt);
void upd_ray();

void prep_LayoutGroup(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_LayoutGroup){
		ttglobal tg;
		ppComponent_Layout p;
		layout_scale_item lsi;
		ttrenderstate rs;
		struct X3D_LayoutGroup *node = (struct X3D_LayoutGroup*)_node;
		tg = gglobal();
		p = (ppComponent_Layout)tg->Component_Layout.prv;
		rs = renderstate();

		//my understanding of layoutgroup: its a child of a) LayoutLayer or b) (another) LayoutGroup
		//except not a normal child, by my interpretation, it has its own layout scales
		//so it needs to undo any scales applied by its parent LayoutLayer/LayoutGroup and
		//do its own. That could be implemented different ways, I chose
		//a scale stack just for LayoutGroup
		lsi = stack_top(layout_scale_item,p->layout_scale_stack);
		FW_GL_PUSH_MATRIX();
		FW_GL_SCALE_F(1.0f/lsi.scale[0],1.0f/lsi.scale[1],1.0f);

		if(node->viewport) prep_Viewport(node->viewport);

		//now it does its own scales
		if(node->layout) prep_Layout(node->layout);
	}
}
void child_LayoutGroup(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_LayoutGroup){
		int ivpvis;
		Stack *vportstack;
		ttglobal tg;
		ttrenderstate rs;
		struct X3D_LayoutGroup *node = (struct X3D_LayoutGroup*)_node;

		rs = renderstate();
		ivpvis = TRUE;
		if(!rs->render_vp && !rs->render_collision){

			tg = gglobal();
			vportstack = (Stack *)tg->Mainloop._vportstack;
			ivpvis = currentviewportvisible(vportstack);
			if(ivpvis)
				setcurrentviewport(vportstack);
		}
		if(ivpvis){

			//see prep_transform for equivalent
			struct X3D_Layout *layout = NULL;
			if(node->layout && node->layout->_nodeType == NODE_Layout)
				layout = (struct X3D_Layout*) node->layout;

			normalChildren(node->children);
		}
	}		
}
void fin_LayoutGroup(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_LayoutGroup){
		ttglobal tg;
		ppComponent_Layout p;
		layout_scale_item lsi;
		ttrenderstate rs;
		struct X3D_LayoutGroup *node = (struct X3D_LayoutGroup*)_node;
		
		if(node->layout) fin_Layout(node->layout);
		if(node->viewport) fin_Viewport(node->viewport);
		tg = gglobal();
		p = (ppComponent_Layout)tg->Component_Layout.prv;
		rs = renderstate();

		FW_GL_POP_MATRIX();

	}
}

void compile_Layout(struct X3D_Node *_node){
	int k;
	struct X3D_Layout *node = (struct X3D_Layout*)_node;
	
	//convert string constants to integer constants,
	// if only one string value, then it applies to both dimensions
	node->_align.p[0] = lookup_layoutmode(node->align.p[0]->strptr);
	k = node->align.n -1;
	node->_align.p[1] = lookup_layoutmode(node->align.p[k]->strptr);
	node->_offsetUnits.p[0] = lookup_layoutmode(node->offsetUnits.p[0]->strptr);
	k = node->offsetUnits.n -1;
	node->_offsetUnits.p[1] = lookup_layoutmode(node->offsetUnits.p[k]->strptr);
	node->_scaleMode.p[0] = lookup_layoutmode(node->scaleMode.p[0]->strptr);
	k = node->scaleMode.n -1;
	node->_scaleMode.p[1] = lookup_layoutmode(node->scaleMode.p[k]->strptr);
	node->_sizeUnits.p[0] = lookup_layoutmode(node->sizeUnits.p[0]->strptr);
	k = node->sizeUnits.n -1;
	node->_sizeUnits.p[1] = lookup_layoutmode(node->sizeUnits.p[k]->strptr);

}
void check_compile_layout_required(struct X3D_Node *node){
	//macro has a return; statement in it
	COMPILE_IF_REQUIRED
}

void prep_Layout(struct X3D_Node *_node){
	//push Layout transform (backward if VF_Viewpoint pass, like Transform prep)
	//http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/layout.html#Layout
	if(_node) {
		float offsetpx[2], differential_scale;
		double scale[2];
		Stack *vportstack;
		ivec4 pvport,vport;
		layout_scale_item lsi;
		struct X3D_Layout *node;
		ttrenderstate rs;
		ttglobal tg;
		ppComponent_Layout p;
		tg = gglobal();
		p = (ppComponent_Layout)tg->Component_Layout.prv;
		rs = renderstate();

		node = (struct X3D_Layout*)_node;
		vportstack = (Stack *)tg->Mainloop._vportstack;
		pvport = stack_top(ivec4,vportstack); //parent context viewport

		// we can/should convert string fields to int flags in a compile_layout
		check_compile_layout_required(_node);

		//size
		if(node->_sizeUnits.p[0] == LAYOUT_PIXEL)
			vport.W =(int)node->size.p[0]; //pixel
		else
			vport.W = (int)(pvport.W * node->size.p[0]); //fraction
		if(node->_sizeUnits.p[1] == LAYOUT_PIXEL)
			vport.H = (int)(node->size.p[1]); //pixel
		else
			vport.H = (int)(pvport.H * node->size.p[1]); //fraction
		
		//align
		if(node->_align.p[0] == LAYOUT_LEFT)
			vport.X = pvport.X;
		else if(node->_align.p[0] == LAYOUT_RIGHT)
			vport.X = (pvport.X + pvport.W) - vport.W;
		else //CENTER
			vport.X = (pvport.X + pvport.W/2) - vport.W/2;

		if(node->_align.p[1] == LAYOUT_BOTTOM)
			vport.Y = pvport.Y;
		else if(node->_align.p[1] == LAYOUT_TOP)
			vport.Y = (pvport.Y + pvport.H) - vport.H;
		else //CENTER
			vport.Y = (pvport.Y + pvport.H/2) - vport.H/2;

		//offset
		if(node->_offsetUnits.p[0] == LAYOUT_PIXEL)
			offsetpx[0] = node->offset.p[0]; //pixel
		else
			offsetpx[0] = pvport.W * node->offset.p[0]; //fraction
		if(node->_offsetUnits.p[1] == LAYOUT_PIXEL)
			offsetpx[1] = node->offset.p[1]; //pixel
		else
			offsetpx[1] = pvport.H * node->offset.p[1]; //fraction


		vport.X += (int)offsetpx[0];
		vport.Y += (int)offsetpx[1];


		if(!rs->render_vp && !rs->render_collision){
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
			setcurrentviewport(vportstack);
			upd_ray();
			//for testing to see the rectangle 
			if(0){
				glEnable(GL_SCISSOR_TEST);
				glScissor(vport.X,vport.Y,vport.W,vport.H);
				glClearColor(1.0f,1.0f,0.0f,.2f); //yellow
				glClear(GL_COLOR_BUFFER_BIT); 
				glDisable(GL_SCISSOR_TEST);
			}
		}


		//scale
		differential_scale = ((float)vport.H/(float)pvport.H)/((float)vport.W/(float)pvport.W);
		scale[0] = scale[1] = 1.0;
		if(node->_scaleMode.p[0] == LAYOUT_PIXEL)
			scale[0] = 2.0/(double)(vport.W); //pixel
		if(node->_scaleMode.p[0] == LAYOUT_WORLD)
			scale[0] = 2.0; //world same as fraction
		else if(node->_scaleMode.p[0] == LAYOUT_FRACTION)
			scale[0] = 2.0; //fraction
		
		if(node->_scaleMode.p[1] == LAYOUT_PIXEL)
			scale[1] = 2.0/(double)(vport.H); //pixel
		if(node->_scaleMode.p[1] == LAYOUT_WORLD)
			scale[1] = 2.0; //world same as fraction
		else if(node->_scaleMode.p[1] == LAYOUT_FRACTION)
			scale[1] = 2.0; //fraction


		//strech post-processing of scale
		if(node->_scaleMode.p[0] == LAYOUT_STRETCH)
			scale[0] = scale[1] * differential_scale;
		if(node->_scaleMode.p[1] == LAYOUT_STRETCH)
			scale[1] = scale[0] * 1.0f/differential_scale;
		node->_scale.p[0] = (float) scale[0];
		node->_scale.p[1] = (float) scale[1];

		//see prep_transform for equivalent
		if(!rs->render_vp ) {
			FW_GL_PUSH_MATRIX();
			FW_GL_SCALE_F(node->_scale.p[0],node->_scale.p[1],1.0f);
			lsi.scale[0] = (float)scale[0];
			lsi.scale[1] = (float)scale[1];
			lsi.scalemode[0] = node->_scaleMode.p[0];
			lsi.scalemode[1] = node->_scaleMode.p[1];
			stack_push(layout_scale_item,p->layout_scale_stack,lsi);
		}else{
			lsi.scale[0] = 1.0f;
			lsi.scale[1] = 1.0f;
			lsi.scalemode[0] = LAYOUT_FRACTION;
			lsi.scalemode[1] = LAYOUT_FRACTION;
			stack_push(layout_scale_item,p->layout_scale_stack,lsi);
		}
	}
}
void fin_Layout(struct X3D_Node *_node){
	//similar to fin_Transform except the transform is reverse of prep_Layout
	if(_node){
		Stack *vportstack;
		struct X3D_Layout *node;
		ttrenderstate rs;
		rs = renderstate();
		ttglobal tg;
		ppComponent_Layout p;
		tg = gglobal();
		p = (ppComponent_Layout)tg->Component_Layout.prv;

		node = (struct X3D_Layout*)_node;



		if(!rs->render_vp && !rs->render_collision){
			vportstack = (Stack *)tg->Mainloop._vportstack;
			popviewport(vportstack);
			setcurrentviewport(vportstack);
			upd_ray();
		}

		stack_pop(layout_scale_item,p->layout_scale_stack);
		if(rs->render_vp) {
			if((node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) {
				FW_GL_SCALE_F(1.0f/node->_scale.p[0],1.0f/node->_scale.p[1],1.0f);
			}
		}else{
			FW_GL_POP_MATRIX();
		}
	}
}
void prep_Layer(struct X3D_Node * _node);
void child_Layer(struct X3D_Node * _node);
void fin_Layer(struct X3D_Node * _node);
//layoutLayer functions are called only by Layerset, not via virtual functions in render_hier()
// - except for fun/testing they can be put in root scene to see what happens
void prep_LayoutLayer(struct X3D_Node *_node){
	struct X3D_LayoutLayer *layoutlayer = (struct X3D_LayoutLayer*)_node;
	prep_Layer(_node); //does the viewport part
	if(layoutlayer->layout)
		prep_Layout(layoutlayer->layout);

}
void child_LayoutLayer(struct X3D_Node *_node){
	child_Layer(_node);
}
void fin_LayoutLayer(struct X3D_Node *_node){
	struct X3D_LayoutLayer *layoutlayer = (struct X3D_LayoutLayer*)_node;
	//pop Layout transform (backward if VF_Viewpoint pass, like Transform fin)
	if(layoutlayer->layout)
		fin_Layout(layoutlayer->layout);
	fin_Layer(_node); //pops the viewport part
}

void prep_ScreenGroup(struct X3D_Node *node){
	if(node && node->_nodeType == NODE_ScreenGroup){
		ttglobal tg;
		Stack *vportstack;
		ivec4 pvport;
		float sx,sy;
		tg = gglobal();
		vportstack = (Stack *)tg->Mainloop._vportstack;
		pvport = stack_top(ivec4,vportstack); //parent context viewport
		
		sx = 1.0f/(float)(pvport.W);
		sy = 1.0f/(float)(pvport.H);
		FW_GL_PUSH_MATRIX();

		/* SCALE */
		FW_GL_SCALE_F(sx,sy,sx);
		//FW_GL_SCALE_F(.01f,.01f,.01f);
	}

}
void child_ScreenGroup(struct X3D_Node *_node){
	struct X3D_ScreenGroup *node = (struct X3D_ScreenGroup*)_node;
	normalChildren(node->children);
}
void fin_ScreenGroup(struct X3D_Node *node){
	if(node && node->_nodeType == NODE_ScreenGroup){
		FW_GL_POP_MATRIX();
	}
}