/*


X3D Picking Component

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

#include "../world_script/fieldSet.h"
#include "../x3d_parser/Bindable.h"
#include "Collision.h"
#include "quaternion.h"
#include "Viewer.h"
#include "../opengl/Frustum.h"
#include "../opengl/Material.h"
#include "../opengl/OpenGL_Utils.h"
#include "../input/EAIHelpers.h"	/* for newASCIIString() */

#include "Polyrep.h"
#include "LinearAlgebra.h"
#include "Component_Picking.h"
#include "Children.h"
#include "RenderFuncs.h"
#ifdef DJTRACK_PICKSENSORS

/* see specifications section 38. Picking Sensor Component */


struct PickStruct {
	void *	tonode;
	void (*interpptr)(void *);
	GLDOUBLE picksensor2world[16],world2picksensor[16];
};


/* DJTRACK_PICKSENSORS */
//struct PickStruct *PickSensors = NULL;
//int num_PickSensors = 0;
//int curr_PickSensor = 0;
//int active_PickSensors = FALSE;
//
//GLDOUBLE viewpoint2world[16];
//int nPickedObjects;


typedef struct pComponent_Picking{
	struct PickStruct *PickSensors;// = NULL;
	int num_PickSensors;// = 0;
	int curr_PickSensor;// = 0;
	int active_PickSensors;// = FALSE;

	GLDOUBLE viewpoint2world[16];
	int nPickedObjects;
	char strprintbits[33];

}* ppComponent_Picking;
void *Component_Picking_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Picking));
	memset(v,0,sizeof(struct pComponent_Picking));
	return v;
}
void Component_Picking_init(struct tComponent_Picking *t){
	//public
	//private
	t->prv = Component_Picking_constructor();
	{
		ppComponent_Picking p = (ppComponent_Picking)t->prv;
		p->PickSensors = NULL;
		p->num_PickSensors = 0;
		p->curr_PickSensor = 0;
		p->active_PickSensors = FALSE;

		//p->viewpoint2world[16];
		//p->nPickedObjects;
		//p->strprintbits[33];
	}
}
//ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;

/* The PickSensors are used via functions, ie the actual data structures should not be exposed outside this file*/

/* call save_viewpoint2world in mainloop at the scengraph root level,
   outside of any render_hier call and after initializing viewpoint for the frame */
void save_viewpoint2world()
{
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, p->viewpoint2world); 

}
//char strprintbits[33];
char *printbits(unsigned short ii)
{
	unsigned short i,j;
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	for(i=0;i<16;i++)
	{
		j = (1 << i) & ii;
		if(j) p->strprintbits[16-i-1] = '1';
		else p->strprintbits[16-i-1] = '0';
	}
	p->strprintbits[16] = 0;
	return p->strprintbits;
}
void pick_PointPickSensor (struct X3D_PointPickSensor *node) { 
 
	int i, index;
	ppComponent_Picking p;
	if(!((node->enabled))) return; 
	p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	//COMPILE_IF_REQUIRED 
 
	/* find which one we are in the picksensor table */
	index = -1;
	for(i=0;i<p->num_PickSensors;i++)
	{
		if(p->PickSensors[i].tonode == node)
		{
			index = i;
			break;
		}
	}
	if( index == -1 )
	{
		//should we add it here?
		//add to table
		//set VF_PickingSensor flag
	}
	if(index > -1)
	{
		/* store picksensor2world transform */
		GLDOUBLE picksensor2viewpoint[16];
		FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, picksensor2viewpoint); 
		
		matmultiplyAFFINE(p->PickSensors[i].picksensor2world,picksensor2viewpoint,p->viewpoint2world);
		//matinverse(PickSensors[i].world2picksensor,PickSensors[i].picksensor2world);

 		/* loop through target nodes and flag them as targets for the next pass*/
		for(i=0;i<node->pickTarget.n;i++)
		{
			struct X3D_Node *t = (struct X3D_Node *)node->pickTarget.p[i];
			if( t ) /* can nodes be deleted */
			{
				unsigned short isIn, pg, pa;
				pg = VF_inPickableGroup;
				//printf("flag before %s\n",printbits((unsigned short)t->_renderFlags)); //%d %o",t->_renderFlags,t->_renderFlags);
				t->_renderFlags = t->_renderFlags | pg; //& (0xFFFF^VF_inPickableGroup);
				//printf("flag  after %s\n",printbits((unsigned short)t->_renderFlags)); //%d %o VF %d %o ",t->_renderFlags,t->_renderFlags,pg,pg);
				//printf("VF_inPickab %s\n",printbits(pg));
				//pa = (0xFFFF^VF_inPickableGroup);
				//printf("ffff ^ pg   %s\n",printbits(pa));
				//pa = (((unsigned short)node->_renderFlags) & pa);
				//printf("flags&pa    %s\n",printbits(pa));
				//isIn = t->_renderFlags & pg;
				//printf(" isIn=%d\n",isIn);
			}
		}
	}

	/* we wont know if we got a hit until after we visit all the pickTargets */
	//(node->__hit) = 1; 
	//(node->isActive) = 1;
 
} 

void do_PickSensorTickDUMMY(void *ptr) {
}
void do_PickingSensorTick ( void *ptr, int nhits ) {
	struct X3D_PointPickSensor *node = (struct X3D_PointPickSensor *)ptr;
	//UNUSED(over);

	/* if not enabled, do nothing */
	if (!node) 
		return;
	//if (node->__oldEnabled != node->enabled) {
	//	node->__oldEnabled = node->enabled;
	//	MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_PointPickSensor, enabled));
	//}
	if (!node->enabled) 
		return;

	/* only do something if there were some hits */
	if (!nhits) return;

	/* set isActive true */
	node->isActive=TRUE;
	MARK_EVENT (ptr, offsetof (struct X3D_PointPickSensor, isActive));

	//if (node->autoOffset) {
	//	memcpy ((void *) &node->offset,
	//		(void *) &node->rotation_changed,
	//		sizeof (struct SFRotation));
	//}
}
void do_pickSensors()
{
	/* called from mainloop > render_pre(), after you have a table of picksensors and pick results. 
	Loop through them, updating each picksensor 
	*/
	int i;
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	for(i=0;i<p->nPickedObjects;i++)
	{
		//do_PickingSensorTick ( node, hits, nhits); // void *ptr, int nhits );
	}
}
/* DJTRACK_PICKSENSORS - modelled on prep_Group above */
/* prep_PickableGroup - we need this so that distance (and, thus, distance sorting) works for PickableGroups */
void prep_PickableGroup (struct X3D_Group *node) {
	/* printf("%s:%d prep_PickableGroup\n",__FILE__,__LINE__); */
	RECORD_DISTANCE
/*
printf ("prep_PickableGroup %p (root %p), flags %x children %d ",node,rootNode,node->_renderFlags,node->children.n);
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
if ((node->_renderFlags & VF_inPickableGroup) == VF_inPickableGroup) printf ("VF_inPickableGroup ");
printf ("\n");
*/
}


/* DJTRACK_PICKSENSORS - modelled on child_Group above */
void child_PickableGroup (struct X3D_Group *node) {
	CHILDREN_COUNT
	//LOCAL_LIGHT_SAVE
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
if ((node->_renderFlags & VF_inPickableGroup) == VF_inPickableGroup) printf ("VF_inPickableGroup ");
printf ("\n");
*/
	RETURN_FROM_CHILD_IF_NOT_FOR_ME
	/* printf("%s:%d child_PickableGroup\n",__FILE__,__LINE__); */

	 if (1==0) {
		int x;
		struct X3D_Node *xx;

		printf ("child_PickableGroup, this %p rf %x isProto %d\n",node,node->_renderFlags, node->FreeWRL__protoDef);
		printf ("	..., render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
			renderstate()->render_vp,renderstate()->render_geom,renderstate()->render_light,
			renderstate()->render_sensitive,renderstate()->render_blend,renderstate()->render_proximity,
			renderstate()->render_collision); 

		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->_sortedChildren.p[x]);
			printf ("	ch %p type %s dist %f\n",node->_sortedChildren.p[x],stringNodeType(xx->_nodeType),xx->_dist);
		}
	}

	/* do we have a DirectionalLight for a child? */
	//LOCAL_LIGHT_CHILDREN(node->_sortedChildren);
	prep_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);

	/* printf ("chld_PickableGroup, for %u, protodef %d and FreeWRL_PROTOInterfaceNodes.n %d\n",
		node, node->FreeWRL__protoDef, node->FreeWRL_PROTOInterfaceNodes.n); */
	/* now, just render the non-directionalLight children */
	if ((node->FreeWRL__protoDef!=INT_ID_UNDEFINED) && renderstate()->render_geom) {
		(node->children).n = 1;
		normalChildren(node->children);
		(node->children).n = nc;
	} else {
		normalChildren(node->_sortedChildren);
	}

	LOCAL_LIGHT_OFF
	fin_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);

}


void chainUpPickableTree(struct X3D_Node *shapeNode ,struct X3D_Node *chained , int status) {

	int possibleStatus ;
	/* status: -1 = indeterminate , 0 = False , 1 = True */
	/* Valid status transitions:
		-1 -> 0
		-1 -> 1

		1 -> 0
	*/

	/* Special cases:
		A branch/leaf has more than one parent:
			Consider the shape unpickable if any parent has pickable as false
		A pickable group is inside a pickable group
			Consider the shape unpickable if any pickable group in a chain has pickable as false
		Pathalogical cases: (According to JAS Jul 2010 these will never happen)
			Chain A contains a node pointing to Chain B, and B contains a node pointing to Chain A
			Similarly A --> B , B --> C , C --> A and  ... --> N ... --> N
			Similarly A --> A but closer to the leaves, ie a self-loop
	*/
	/* Invalid status transitions:
		Cannot become indeterminate
		0 -> -1
		1 -> -1 

		Take care of the non-Pathalogical special cases
		0 -> 1
	*/
	/* This will happen if you complete parent X and are going up parent Y */
	if (status == 0) return ;

	possibleStatus = status ;
	if (chained->_nodeType == NODE_PickableGroup) {
		/* OK, we have a PickableGroup and this might change the status flag */

		/* Sorry, I do not have the shorthand for this ;( */
		struct X3D_PickableGroup *pickNode ;
		pickNode = (struct X3D_PickableGroup *) chained ;
		possibleStatus = pickNode->pickable ;
		
		if (status == -1) {
			/* dump_scene (stdout, 0, chained) ; */
			status = possibleStatus ;
			if(possibleStatus == 0) {
				shapeNode->_renderFlags = shapeNode->_renderFlags & (0xFFFF^VF_inPickableGroup);
				/* No point in going further up this tree, we have gone from -1 -> 0 */
			} else {
				shapeNode->_renderFlags = shapeNode->_renderFlags | VF_inPickableGroup;
				if(0 != chained->_nparents) {
					int i;
					for(i=0;i<chained->_nparents;i++) {
						chainUpPickableTree(shapeNode, chained->_parents[i], status) ;
					}
				}
			}
		} else {
			/* The current status is 1 */
			if(possibleStatus == 0) {
				shapeNode->_renderFlags = shapeNode->_renderFlags & (0xFFFF^VF_inPickableGroup);
				/* No point in going further up this tree, we have gone from 1 -> 0 */
			} else {
				/* Still 1, so no change to _renderFlags */
				if(0 != chained->_nparents) {
					int i;
					for(i=0;i<chained->_nparents;i++) {
						chainUpPickableTree(shapeNode, chained->_parents[i], status) ;
					}
				}
			}
		}
	} else {
		if(0 != chained->_nparents) {
			int i;
			for(i=0;i<chained->_nparents;i++) {
				chainUpPickableTree(shapeNode, chained->_parents[i], status) ;
			}
		}
	}
	return ;
}






/* DJTRACK_PICKSENSORS */
void activate_picksensors() { 
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;

	p->active_PickSensors = TRUE ; }
void deactivate_picksensors() { 
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;

	p->active_PickSensors = FALSE ; }

int enabled_picksensors() 
{
  int i;
  int someEnabled = FALSE;
  ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;

    for(i=0;i<p->num_PickSensors;i++) {
        if (checkNode(p->PickSensors[i].tonode,__FILE__,__LINE__)) {
        someEnabled = someEnabled || ((struct X3D_PointPickSensor *)(p->PickSensors[i].tonode))->enabled;
        }
    }
  return someEnabled;
 }

int  active_picksensors() { 
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	return (p->active_PickSensors && (p->num_PickSensors > 0)) ; }
void rewind_picksensors() { 
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	p->curr_PickSensor = 0 ; }
void advance_picksensors() {
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	p->curr_PickSensor++; }
int  more_picksensors() {
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	if (p->active_PickSensors && p->curr_PickSensor < p->num_PickSensors) {
		return TRUE ;
	} else {
		return FALSE ;
	}
}

void pick_Sphere (struct X3D_Sphere *node) {

	GLDOUBLE shape2viewpoint[16],viewpoint2world[16],shape2world[16],world2shape[16],shape2picksensor[16], picksensor2shape[16];
	GLDOUBLE *picksensor2world;
	GLDOUBLE radiusSquared;
	int i,j;
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;

	/* this sucker initialized yet? */
	if (node->__points.n == 0) return;
	// check before render_hier if(!num_picksensors) return;

	/* get the transformed position of the Sphere, and the scale-corrected radius. */
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, shape2viewpoint);
	matmultiplyAFFINE(shape2world,shape2viewpoint,viewpoint2world);
	matinverseAFFINE(world2shape,shape2world);

	for(i=0;i<p->num_PickSensors;i++)
	{
		struct X3D_Node *picksensor = p->PickSensors[i].tonode;

        if (checkNode(picksensor,__FILE__,__LINE__)) {

            picksensor2world = p->PickSensors[i].picksensor2world;
		
            matmultiplyAFFINE(shape2picksensor,picksensor2world,world2shape);
            switch (picksensor->_nodeType) {
                case NODE_PointPickSensor:  
                {
                    struct X3D_PointPickSensor *pps = (struct X3D_PointPickSensor*  )picksensor;
				
                    struct X3D_PointSet *points = (struct X3D_PointSet *)pps->pickingGeometry;
                    for(j=0;j<points->attrib.n;j++)
                    {
                        struct point_XYZ pickpoint;
                        transform(&pickpoint,(struct point_XYZ *)&points->attrib.p[j],picksensor2shape);
                        radiusSquared = vecdot(&pickpoint,&pickpoint);
                        if( radiusSquared < node->radius )
                        {
                            printf("bingo - we have a hit ");
                            /* according to specs, if we should report the intersection point then transform it into picksensor space */
                            transform(&pickpoint,&pickpoint,shape2picksensor);
                            printf(" at %lf %lf %lf in picksensor space\n",pickpoint.x,pickpoint.y,pickpoint.z);
                        }

                    }
                    break;
                }
            }
        }
	}
}



/* DJTRACK_PICKSENSORS */
void add_picksensor(struct X3D_Node * node) {
	void (*myp)(void *);
	int clocktype;
	int count;
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	
	if (node == 0) {
		printf ("error in add_first; somehow the node datastructure is zero \n");
		return;
	}

	clocktype = node->_nodeType;
	/* printf ("add_picksensor for %s\n",stringNodeType(clocktype)); */

	if (NODE_PointPickSensor == clocktype) {
		printf ("add_picksensor for %s\n",stringNodeType(clocktype));
		myp =  do_PickSensorTickDUMMY;
	} else {
		/* printf ("this is not a type we need to add_first for %s\n",stringNodeType(clocktype)); */
		return;
	}
	p->PickSensors = (struct PickStruct *)REALLOC(p->PickSensors,sizeof (struct PickStruct) * (p->num_PickSensors+1));
	if (p->PickSensors == 0) {
		printf ("can not allocate memory for add_first call\n");
		p->num_PickSensors = 0;
	}

	/* does this event exist? */
	for (count=0; count <p->num_PickSensors; count ++) {
		if (p->PickSensors[count].tonode == node) {
			/* printf ("add_first, already have %d\n",node); */
			return;
		}	
	}

	/* now, put the function pointer and data pointer into the structure entry */
	p->PickSensors[p->num_PickSensors].interpptr = myp;
	p->PickSensors[p->num_PickSensors].tonode = node;

	p->num_PickSensors++;
}

/* DJTRACK_PICKSENSORS */
struct X3D_Node* get_picksensor() {
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	int this_PickSensor = p->curr_PickSensor ;
	if ( p->active_PickSensors && this_PickSensor < p->num_PickSensors) {
		return p->PickSensors[this_PickSensor].tonode ;
	} else {
		return (struct X3D_Node *) NULL ;
	}
}
//extern int render_picksensors;
//extern int render_pickables;
void other_PointPickSensor (struct X3D_PointPickSensor *node) 
{
	if(renderstate()->render_picksensors)
		pick_PointPickSensor(node);
}
void other_PickableGroup (struct X3D_Group *node) 
{
}
void other_Sphere (struct X3D_Sphere *node) 
{
	if(renderstate()->render_pickables)
		pick_Sphere(node);
}

void remove_picksensor(struct X3D_Node * node) {}
#else // DJTRACK_PICKSENSORS
struct nodedistance {
	struct X3D_Node* node;
	float dist;
};

typedef struct pComponent_Picking{
	//Stack *stack_nodesintersected;
	Stack *stack_nodesdistance;
	Stack *stack_intersections;
	Stack *stack_pointsinside;
}* ppComponent_Picking;
void *Component_Picking_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Picking));
	memset(v,0,sizeof(struct pComponent_Picking));
	return v;
}
void Component_Picking_init(struct tComponent_Picking *t){
	//public
	//private
	t->prv = Component_Picking_constructor();
	{
		ppComponent_Picking p = (ppComponent_Picking)t->prv;
		p->stack_intersections = newStack(struct intersection_info);
		//p->stack_nodesintersected = newStack();
		p->stack_nodesdistance = newStack(struct nodedistance);
		p->stack_pointsinside = newStack(struct intersection_info);
	}
}
//ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;


/* PICKSENSOR stubs */
void other_PointPickSensor (struct X3D_PointPickSensor *node) {}
void other_PickableGroup (struct X3D_Group *node) {}
void other_Sphere (struct X3D_Sphere *node) {}
//void child_PickableGroup (struct X3D_Group *node) {}
void prep_PickableGroup (struct X3D_Group *node) {}
void add_picksensor(struct X3D_Node * node) {}
void remove_picksensor(struct X3D_Node * node) {}

void push_pickablegroupdata(void *userdata);
void pop_pickablegroupdata();
void child_PickableGroup (struct X3D_Group *node) {
	CHILDREN_COUNT
	RETURN_FROM_CHILD_IF_NOT_FOR_ME
	/* printf("%s:%d child_PickableGroup\n",__FILE__,__LINE__); */

	prep_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);


	//PUSH OBJECTTYPE
	//PUSH PICKABLE == TRUE/FALSE
	push_pickablegroupdata(node);
	
	normalChildren(node->children);

	//POP PICKABLE == TRUE/FALSE
	//POP OBJECTTTYPE
	pop_pickablegroupdata();
	fin_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);
}


#endif // DJTRACK_PICKSENSORS

int overlapMBBs(GLDOUBLE *MBBmin1, GLDOUBLE *MBBmax1, GLDOUBLE *MBBmin2, GLDOUBLE* MBBmax2);
/*
A. Q. Should we even bother to implement Picking component? Few others are:
	http://www.web3d.org/wiki/index.php/Player_support_for_X3D_components
	- Xj3D and Flux/Vivaty listed as supporting picking component
	x problems running xj3d on win10 - doesn't run for dug9
	x neither vivaty studio nor vivatyplayer have it, just something called PickGroup with enable field
	x cobweb not listed, but on their site they don't mention Picking component
	* dune has the nodes, 
		x but doesn't seem to generate proper parent-child at design-time: 
		- picksensor.pickingGeometry should be a geometry node
		* but does load a scene designed elsewhere ie by hand in text editor

B. New theory of operation for PickSensors (July 2016, dug9):
1. like the new do_TransformSensor - all you need is do_PickSensorTick and VF_USE and usehit_ functions.
	- picksensor and target nodes are flagged here with VF_USE
	- then on next frame/tick the picksensor and targets are recovered here with usehit_ functions, 
		including their modelview transform
	- and all the work is done here in do_PickSensorTick
	- (no need for compile_ other_ etc, not yet)
2. we could simplify by doing BOUNDS only:
	- if target is geom, then can do BOUNDS or GEOMETRY intersection
	- if target is 'chunk of scenegraph' ie Group, Transform or PickableGroup, then 
		for now july 2016: we do only BOUNDS, 
			- taking only the _extent of the (topmost) target node, 
			- not delving/probing into children/descendants
		future: do a special setup of render_node(node) and children recursion for picking that examines each descendant
			- and/or over-ride rayhit (mouse picking) functions
			- push and pop X3DPickable state according to X3DPickableGroup in the transform hierarchy
	- ^^the pickingGeometry SFNode is not a USE reference, it has only this node as parent (or we'll ignore any other USE modelview matrix)
3. what 'space' to work in, given 3 parts: PickSensor, pickingGeometry, pickTarget:
	- transformsensor did its work in transformsensor node space (where the transformsensor was in scenegraph)
		- so any evenouts were in transformsensor space
	- in theory for picksensors we could do our work in 
			a) world space (by taking view off modelview matrix, and doing one-way transforms local2world for all 3 parts), or 
			b) pickTarget space (as we do with mouse picking, transforming the pickray on each transform stack push), or 
			c) pickingGeometry space (we'll assume^^ this is the same space as d) picksensor)
			d) pickSensor space (like transformSensor)
		-each option would/could affect function design
	- for now we'll do work in d) picksensor space, so 'closest' is easy to sort relative to 0,0,0
		and that means transforming pickTarget to picksensor space 
		and assuming^^ pickingGeometry is already in picksensor space
HARD PARTS:
	1. picking against a partial transform hierarchy of objects
		- can use the USEUSE approach to get the starting transform stack for target node
		- need to modify render_node() and/or normalChildren and/or render(node0 for picking partial pass
	2. geom-geom intersections
		- bounding box easy
		- we do ray-geom for touchsensors
		- we do 'avatar brush' (bunch of line segments) vs geom in collision
		- RBP will do geom-geom collisions, but the geoms are simplifications: box, sphere
		- if transform pickee into picker space, then simple math formulas 
			ie cone if bottomRadius is R, and height is H, and and a point is between bottom and top
			at hieght h from bottom, then cone r = f(h,R,H) = R*(1-h/H)
		- convex hull - particlephysics we did that for polyrep
	
*/
int objecttypes_overlap(struct Multi_String * list1, struct Multi_String * list2, struct Uni_String * criterion){
	int ntries, nmatches, iallsensor, ialltarget,inonetarget;
	int iret = FALSE;
	//hope I got this logic right
	ntries = nmatches = iallsensor = ialltarget = inonetarget = 0;
	for(int j=0;j<list2->n;j++){
		if(!strcmp(list2->p[j]->strptr,"ALL")) ialltarget = TRUE;
		if(!strcmp(list2->p[j]->strptr,"NONE")) inonetarget = TRUE;
	}

	for(int i=0;i<list1->n;i++){
		if(!strcmp(list1->p[i]->strptr,"ALL")) iallsensor = TRUE;
		for(int j=0;j<list2->n;j++){
			ntries++;
			if(!strcmp(list1->p[i]->strptr,list2->p[j]->strptr)) {
				nmatches++;
			}
		}
	}
	if(!strcmp(criterion->strptr,"MATCH_ANY")){
		if(nmatches) iret = TRUE;
	}else if(!strcmp(criterion->strptr,"MATCH_ALL")){
		if(nmatches == ntries) iret = TRUE;
	}else if(!strcmp(criterion->strptr,"MATCH_ONE")){
		if(nmatches == 1) iret = TRUE;
	}
	if(iallsensor || ialltarget) iret = TRUE;
	if(inonetarget) iret = FALSE;
	return iret;
}
int isGeometryNode(struct X3D_Node* node){
	//is there a bitflag or other function somewhere to say if a node is a geometry type node?
	int iret = FALSE;
	struct X3D_Virt *virt = virtTable[node->_nodeType];
	if(virt->rend || virt->rendray) iret = TRUE;
	return iret;
}
//struct nodedistance {
//	struct X3D_Node* node;
//	float dist;
//};
//Compare function 
//return value	Description
//< 0			elem1 less than elem2
//  0			elem1 equivalent to elem2
//> 0			elem1 greater than elem2
int compare_nodedistance(const void *elem1,const void * elem2 ) 
{  
   struct nodedistance *nd1 = (struct nodedistance *)elem1; 
   struct nodedistance *nd2 = (struct nodedistance *)elem2;
   return nd1->dist < nd2->dist ? -1 : nd1->dist > nd2->dist ? 1 : 0;  
}  
int compare_intersectiondistance(const void *elem1, const void * elem2){
   struct intersection_info *nd1 = (struct intersection_info *)elem1; 
   struct intersection_info *nd2 = (struct intersection_info *)elem2;
   return nd1->dist < nd2->dist ? -1 : nd1->dist > nd2->dist ? 1 : 0;  
}
void do_PickSensorTick(void *ptr){
	//heavy borrowing from do_TransformSensor
	int ishit,i,j;
	usehit *mehit, *uhit;
	struct X3D_Node *unode,*menode, *pnode;
	struct Multi_Node *unodes;
	//we'll use PrimitivePickSensor for the generic picksensor type 
	// -the order of field definitions for all picksensors must be the same in Perl code generator VRMLNodes.pm
	//  up to the point of per-type differences
	ppComponent_Picking p = (ppComponent_Picking)gglobal()->Component_Picking.prv;
	struct X3D_PrimitivePickSensor *node = (struct X3D_PrimitivePickSensor *) ptr;
	switch(node->_nodeType){
		case NODE_LinePickSensor:
		case NODE_PointPickSensor:
		case NODE_PrimitivePickSensor:
		case NODE_VolumePickSensor:
		break;
		default:
			return; //not for me
	}

	// if not enabled, do nothing 
	if (!node) return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_PrimitivePickSensor, enabled));
	}
	// are we enabled? 
	if (!node->enabled) return;

	#ifdef SEVERBOSE
	printf ("do_TransformSensorTick enabled\n");
	#endif

	//temp clear hit flag
	ishit = 0;
	mehit = NULL;
	unodes = &node->pickTarget;
	menode = (struct X3D_Node*)node; //upcaste
	pnode = node->pickingGeometry;
	//naming: 
	//'me' is the pickingsensor node
	//'u' is a pick target node
	if(unodes->n && pnode){
		//check all USE-USE combinations of this node and pickTargets
		//find ME: the picksensor, in the usehit list
		while(mehit = usehit_next(menode,mehit)){  //hopefully there's only one instance of me/picksensor node in the scenegraph
			int iret;
			double meinv[16],memin[3],memax[3];
			float emin[3], emax[3], halfsize[3];

			matinverseAFFINE(meinv,mehit->mvm);
			//iret = __gluInvertMatrixd( mehit->mvm, meinv);

			if(0){
				//check inverse
				double ident[16];
				int j;
				matmultiplyAFFINE(ident,meinv,mehit->mvm);

				printf("inverse check do_TransformSensor\n");
				for(i=0;i<4;i++){
					for(j=0;j<4;j++) printf("%lf ",ident[i*3+j]);
					printf("\n");
				}
				printf("\n");
			}
			//update extent on me, in case center or size has changed
			for(i=0;i<3;i++)
			{
				emin[i] = pnode->_extent[i*2 + 1];
				emax[i] = pnode->_extent[i*2];
			}
			for(i=0;i<3;i++)
			{
				node->_extent[i*2 + 1] = emin[i];
				node->_extent[i*2]     = emax[i];
			}
			for(i=0;i<3;i++)
			{
				memin[i] = node->_extent[i*2 + 1];
				memax[i] = node->_extent[i*2];
			}

			//find U: a target/pickable in the usehit list
			p->stack_intersections->n = 0; //stack_clear
			p->stack_nodesdistance->n = 0; //stack_clear
			p->stack_pointsinside->n = 0; //stack_clear
			uhit = NULL;
			for(j=0;j<unodes->n;j++){
				unode = unodes->p[j];
				while(uhit = usehit_next(unode,uhit)){
					//see if they intersect, if so do something about it
					//-prepare matrixTarget2this
					int intypes,pickable;
					double u2me[16], me2u[16], umin[3],umax[3],uumin[3],uumax[3];

					struct X3D_PickableGroup *pgroup;
					pgroup = (struct X3D_PickableGroup *) uhit->userdata;
					intypes = TRUE;
					pickable = TRUE;
					if(pgroup){
						pickable = pgroup->pickable;
						intypes = objecttypes_overlap(&node->objectType,&pgroup->objectType,node->matchCriterion);
					}
					if(intypes && pickable){
						matmultiplyAFFINE(u2me,uhit->mvm,meinv);
						matinverseAFFINE(me2u,u2me);
						//-transform target AABB/MBB from target space to this space
						//the specs say it should be done in world space, and perhaps there, 
						//.. normally the MBB/AABB will be aligned to world, as the scene author is thinking
						//.. but we'll do our MBB/AABB test in picksensor space for now,
						//.. to save a step
						for(i=0;i<3;i++)
						{
							umin[i] = unode->_extent[i*2 + 1];
							umax[i] = unode->_extent[i*2];
						}
						transformMBB(uumin,uumax,u2me,umin,umax); 
						//-see if AABB intersect
						if( overlapMBBs(memin, memax, uumin, uumax) ){
							//-if so take further action:
							//(not implemented july 17, 2016 - end of day, no time left, 
							// ..but it does get in here, showing the above plumbing is working)
							//picknode-specific intersections with various targetnode types
							//if further testing shows they intersect, then ishit++:
							if(!strcmp(node->intersectionType->strptr,"BOUNDS") || unode->_nodeType == NODE_Inline){
								struct nodedistance ndist;
								double c1[3],c2[3],dd[3];
								//stack_push(struct X3D_Node*,p->stack_nodesintersected,unode);
								vecaddd(c1,memin,memax);
								vecscaled(c1,c1,.5);
								vecaddd(c2,umin,umax);
								vecscaled(c2,c2,.5);
								vecdifd(dd,c2,c1);
								ndist.dist = veclengthd(dd);
								ndist.node = unode;
								stack_push(struct nodedistance,p->stack_nodesdistance,ndist);
								ishit++;
							}else if(!strcmp(node->intersectionType->strptr,"GEOMETRY")){
								//we need to traverse the scenegraph subsection to get to the geometry
								//clear some global variables
								//set some render flags
								//call render(unode) and bottom out on a callback above, which will
								// do the work and set global variables
								//retrieve global variables
								//options:
								//a) the callback snapshots the geometry node and matrix in a uhit,
								//   and we get back (yet another) list of uhits
								//b) the callback does the intersections, and gives us back 
								//   the intersection lists
								//decision: lets do a) here
								usehitB_clear();
								Stack *usehitB;
								double viewMatrix[16];

								if(isGeometryNode(unode)){
									double matidentity[16];
									loadIdentityMatrix(matidentity);
									usehitB_add2(unode,matidentity,pgroup);
									loadIdentityMatrix(viewMatrix);
								}else{
									//snapshot matrix stack before renderhier
									//- we're at the world level, so viewpoint will be in the matrix stack
									FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, viewMatrix);
									render_hier(unode, VF_Geom | VF_Picking);
								}
								usehitB = getUseHitBStack();
								for(int m=0;m<vectorSize(usehitB);m++){
									//geometry node
									double u2meg[16], me2ug[16], nodedistance;
									usehit *ghit = vector_get_ptr(usehit,usehitB,m);

									// concatonate matrices 
									// u2meg = gmat * umat * meinv
									//       = gmat * u2me
									{
										double viewinv[16], world2geom[16];
										//take off the viewmatrix from the geometry matrix
										matinverseAFFINE(viewinv,viewMatrix);
										//now get the matrix to go from ME to UGeom
										matmultiply(world2geom,viewinv,ghit->mvm);
										matmultiplyAFFINE(me2ug,world2geom,me2u);
										matinverseAFFINE(u2meg,me2ug);
									}


									switch(node->_nodeType){
										case NODE_LinePickSensor:
										{
											//foreach line segment:
											// transform ends into U
											// intersect with U geometry
											// sort by distance
											// transform intersections and normals back
											// accumulate intersections
											//compile_LinePick_sensor
											float *segments = NULL; //a segment has 2 points / 6 floats
											int nseg = 0;
											switch(pnode->_nodeType){
												case NODE_IndexedLineSet:
												{
													struct X3D_IndexedLineSet *lnode = (struct X3D_IndexedLineSet *)pnode;
													segments = MALLOC(float*,lnode->coordIndex.n * 2 * 3 * sizeof(float));
													float *points = (float*)((struct X3D_Coordinate*)lnode->coord)->point.p;
													for(int ik=0;ik<lnode->coordIndex.n;ik++){
														if(lnode->coordIndex.p[ik] == -1) continue;
														if(lnode->coordIndex.p[ik+1] == -1) continue;
														veccopy3f(&segments[6*nseg +0],&points[3*lnode->coordIndex.p[ik]]);
														veccopy3f(&segments[6*nseg +1],&points[3*lnode->coordIndex.p[ik+1]]);
														nseg++;
													}
												}
												break;
												case NODE_LineSet:
												{
													struct X3D_LineSet *lnode = (struct X3D_LineSet *)pnode;
													int nn = 0;
													for(int ik=0;ik<lnode->vertexCount.n;ik++)
														nn += lnode->vertexCount.p[ik];
													segments = MALLOC(float*,nn * 2 * 3 * sizeof(float));
													float *points = (float*)((struct X3D_Coordinate*)lnode->coord)->point.p;
													int kk=0;
													for(int ik=0;ik<lnode->vertexCount.n;ik++){
														for(int jk=0;jk<lnode->vertexCount.p[ik]-1;jk++){
															veccopy3f(&segments[6*nseg +0],&points[3*(kk+jk)]);
															veccopy3f(&segments[6*nseg +3],&points[3*(kk+jk+1)]);
															nseg++;
														}
														kk+= lnode->vertexCount.p[ik];
													}

												}
												break;
												default:
												break;
											}
											float cumdist = 0.0f;
											int cumcount = 0;
											for(int ik=0;ik<nseg;ik++){
												float p1[3], p2[3];
												double dd[3];
												veccopy3f(p1, &segments[6*ik]);
												veccopy3f(p2,&segments[6*ik+3]);
												float2double(dd,p1,3);
												transformAFFINEd(dd,dd,me2ug);
												double2float(p1,dd,3);
												float2double(dd,p2,3);
												transformAFFINEd(dd,dd,me2ug);
												double2float(p2,dd,3);
												//printf("p1,p2 in cylinder space: [%f %f %f][%f %f %f]\n",
												//	p1[0],p1[1],p1[2],p2[0],p2[1],p2[2]);
												if(intersect_polyrep2(ghit->node, p1, p2, p->stack_intersections )){
													float delta[3];
													for(int jk=cumcount;jk<p->stack_intersections->n;jk++){
														struct intersection_info *iinfo = vector_get_ptr(struct intersection_info,p->stack_intersections,jk);
														iinfo->dist += cumdist;
														float2double(dd,iinfo->p,3);
														transformAFFINEd(dd,dd,u2meg);
														//in theory normals should be inverse transpose,
														//we'll cheat here for now, and _you_ the reader, should fix
														double2float(iinfo->p,dd,3);
														float2double(dd,iinfo->normal,3);
														transformUPPER3X3d(dd,dd,u2meg);
														double2float(iinfo->normal,dd,3);
													}
													cumdist += veclength3f(vecdif3f(delta,p2, p1));
													cumcount = p->stack_intersections->n;
												}

											}
											if(cumcount) {
												struct nodedistance ndist;
												struct intersection_info *iinfo;

												ndist.node = unode;
												//for node distance for geometry > lines we'll take the 
												//closest distance along the line that the node intersects
												qsort(p->stack_intersections->data,p->stack_intersections->n, sizeof(struct intersection_info), compare_intersectiondistance );  
												iinfo = vector_get_ptr(struct intersection_info,p->stack_intersections,0);
												ndist.dist = iinfo->dist;
												//stack_push(struct X3D_Node*,p->stack_nodesintersected,unode);
												stack_push(struct nodedistance,p->stack_nodesdistance,ndist);
												ishit++;
											}
											FREE_IF_NZ(segments);
										}
										break;
										case NODE_PointPickSensor:
										{
											//for each picksensor point:
											//  transform into U
											//  create plumbline
											//  intersect plumbline with target geom and see if intersection count is odd
											//  if odd (point inside) accumulate geometry list
											float *points;
											int npoints;
											struct X3D_PointSet *ps = (struct X3D_PointSet*)pnode;
											struct X3D_Coordinate *cc = (struct X3D_Coordinate *)ps->coord;
											points = (float*)cc->point.p;
											npoints = cc->point.n;

											float cumdist = 0.0f;
											int cumcount = 0;
											for(int ik=0;ik<npoints;ik++){
												float p1[3], p2[3];
												double dd[3];
												veccopy3f(p1, &points[3*ik]);
												float2double(dd,p1,3);
												transformAFFINEd(dd,dd,me2ug);
												double2float(p1,dd,3);
												veccopy3f(p2,p1);
												p2[3] = unode->_extent[4] - 1.0; //plumbline point must be guaranteed outside target geom

												//printf("p1,p2 in cylinder space: [%f %f %f][%f %f %f]\n",
												//	p1[0],p1[1],p1[2],p2[0],p2[1],p2[2]);
												int ixcount;
												if(ixcount = intersect_polyrep2(ghit->node, p1, p2, p->stack_intersections )){
													if(ixcount % 2){
														//if odd number of intersections, then the point is inside
														struct intersection_info iinfo;
														float delta[3];
														cumcount++;
														double c1[3], pointdist;
														//stack_push(struct X3D_Node*,p->stack_nodesintersected,unode);
														vecaddd(c1,memin,memax);
														vecscaled(c1,c1,.5);
														double2float(delta,c1,3);
														pointdist = veclength3f(vecdif3f(delta,delta,p1));
														iinfo.dist = pointdist;
														veccopy3f(iinfo.p,&points[3*ik]); //the point inside
														stack_push(struct intersection_info,p->stack_pointsinside,iinfo);
													}
												}

											}
											if(cumcount) {
												struct nodedistance ndist;
												ndist.node = unode;
												ndist.dist = 0.0; //no distance to node required in specs for pointpicksensor
												stack_push(struct nodedistance,p->stack_nodesdistance,ndist);
												ishit++;
											}
										}
										break;
										case NODE_PrimitivePickSensor:
										{
											//for each target geometry vertex:
											//  transform into picksensor space
											//  test if inside geometry
											//  accumulate list
											struct X3D_PolyRep* pr = (struct X3D_PolyRep*)node->_intern;
											if(pr){
												float *points = pr->actualCoord;
												int npts = pr->ntri;
												int cumcount = 0;

												for(int ik=0;ik<npts;ik++){
													double dd[3];
													float pp[3];
													float2double(dd,&points[ik*3],3);
													transformAFFINEd(dd,dd,u2meg);
													double2float(pp,dd,3);
													switch(pnode->_nodeType){
														case NODE_Cone:
														{
															float R,H,h,rc,rp;
															struct X3D_Cone * cone = (struct X3D_Cone*)pnode;
															H = cone->height;
															R = cone->bottomRadius;
															if(pp[1] >= -H/2.0f && pp[1] < H/2.0f){
																float xz[2];
																h = pp[1] - (-H/2.0);
																rc = R*(1.0 - h/H);
																xz[0] = pp[0];
																xz[1] = pp[2];
																rp = veclength2f(xz);
																if(rp <= rc){
																	//inside the cone
																	struct intersection_info iinfo;
																	float apex[3], delta[3], conedist;
																	apex[1] = H/2.0;
																	apex[0] = apex[2] = 0.0f;
																	conedist = veclength3f(vecdif3f(delta,pp,apex));
																	iinfo.dist = conedist;
																	veccopy3f(iinfo.p,pp); //the point inside
																	stack_push(struct intersection_info,p->stack_pointsinside,iinfo);
																}
																
															}
														}
														break;
														case NODE_Cylinder:
														case NODE_Sphere:
														case NODE_Box:
														default:
															break;
													}
												}
												cumcount = p->stack_pointsinside->n;
												if(cumcount) {
													struct nodedistance ndist;
													struct intersection_info *iinfo;
													ndist.node = unode;
													qsort(p->stack_intersections->data,p->stack_intersections->n, sizeof(struct intersection_info), compare_intersectiondistance );  
													iinfo = vector_get_ptr(struct intersection_info,p->stack_intersections,0);

													ndist.dist = iinfo->dist; //no distance to node required in specs for pointpicksensor

													stack_push(struct nodedistance,p->stack_nodesdistance,ndist);
													ishit++;
												}

											}
										}
										break;
										case NODE_VolumePickSensor:
										{
											//for each target geometry vertex
											//  transform into picksensor space
											//  make plubline
											//  test if intersections of plubline with pickgeometry are odd
											//  if odd (inside) accumulate list
											ishit++;
										}
										break;
										default:
											break; //not for me
									}
								} //for each subscenegraph node
							} //end else GEOMETRY
						} //if overlap
					} //if intypes and pickable
				}  //while uhit
			} //for unodes
		} //while mehit
		if(ishit){
			if (!node->isActive) {
				#ifdef SEVERBOSE
				printf ("transformensor - now active\n");
				#endif

				node->isActive = 1;
				MARK_EVENT (ptr, offsetof(struct X3D_PrimitivePickSensor, isActive));
			}
			//sort by sortOrder
			if(!strcmp(node->sortOrder->strptr,"ANY")){
				struct nodedistance *ndist;
				realloc(node->pickedGeometry.p,1 * sizeof(struct X3D_Node*));
				ndist = vector_get_ptr(struct nodedistance,p->stack_nodesdistance,0);
				node->pickedGeometry.p[0] = ndist->node;
			}else if(!strcmp(node->sortOrder->strptr,"ALL")){
				struct nodedistance *ndist;
				realloc(node->pickedGeometry.p,p->stack_nodesdistance->n * sizeof(struct X3D_Node*));
				for(int ii=0;ii<p->stack_nodesdistance->n;ii++){
					ndist = vector_get_ptr(struct nodedistance,p->stack_nodesdistance,0);
					node->pickedGeometry.p[ii] = ndist->node;
				}
				//memcpy(node->pickedGeometry.p,p->stack_nodesintersected->data,p->stack_nodesintersected->n*sizeof(struct X3D_Node*));
			}else if(!strcmp(node->sortOrder->strptr,"ALL_SORTED")){

				//int compare( (void *) & elem1, (void *) & elem2 ); 
				//stdlib.h
				//void qsort(  
				//   void *base,  
				//   size_t num,  
				//   size_t width,  
				//   int (__cdecl *compare )(const void *, const void *)   
				//);  
				struct nodedistance *ndist;

				qsort(p->stack_nodesdistance->data,p->stack_nodesdistance->n, sizeof(struct nodedistance), compare_nodedistance );  
				realloc(node->pickedGeometry.p,p->stack_nodesdistance->n * sizeof(struct X3D_Node*));
				for(int ii=0;ii<p->stack_nodesdistance->n;ii++){
					ndist = vector_get_ptr(struct nodedistance,p->stack_nodesdistance,0);
					node->pickedGeometry.p[ii] = ndist->node;
				}
			}else if(!strcmp(node->sortOrder->strptr,"CLOSEST")){
				struct nodedistance *ndist;

				qsort(p->stack_nodesdistance->data,p->stack_nodesdistance->n, sizeof(struct nodedistance), compare_nodedistance );  
				realloc(node->pickedGeometry.p,1 * sizeof(struct X3D_Node*));
				ndist = vector_get_ptr(struct nodedistance,p->stack_nodesdistance,0);
				node->pickedGeometry.p[0] = ndist->node;
			}
			MARK_EVENT(ptr,offsetof(struct X3D_PrimitivePickSensor, pickedGeometry));
			//MARK_EVENT - pickedGeometry (all)
			//MARK_EVENT - pickedPoint (line and point)
			//MARK_EVENT - pickedNormal (line)
			//MARK_EVENT - pickedTextureCoordinate (line)
			switch(node->_nodeType){
				case NODE_LinePickSensor:
				{
					struct X3D_LinePickSensor *lnode = (struct X3D_LinePickSensor *)node;
					lnode->pickedPoint.n = p->stack_intersections->n;
					lnode->pickedNormal.n = p->stack_intersections->n;
					lnode->pickedTextureCoordinate.n = p->stack_intersections->n;
					lnode->pickedPoint.p = realloc(lnode->pickedPoint.p,lnode->pickedPoint.n * 3 * sizeof(float));
					lnode->pickedNormal.p = realloc(lnode->pickedNormal.p,lnode->pickedPoint.n * 3 * sizeof(float));
					lnode->pickedTextureCoordinate.p = realloc(lnode->pickedTextureCoordinate.p,lnode->pickedPoint.n * 3 * sizeof(float));
					for(int ik=0;ik<p->stack_intersections->n;ik++){
						struct intersection_info *iinfo = vector_get_ptr(struct intersection_info,p->stack_intersections,ik);
						veccopy3f(lnode->pickedPoint.p[ik].c,iinfo->p);
						veccopy3f(lnode->pickedNormal.p[ik].c,iinfo->normal);
						veccopy3f(lnode->pickedTextureCoordinate.p[ik].c,iinfo->texcoord);
					}
					MARK_EVENT(ptr,offsetof(struct X3D_LinePickSensor, pickedPoint));
					MARK_EVENT(ptr,offsetof(struct X3D_LinePickSensor, pickedNormal));
					MARK_EVENT(ptr,offsetof(struct X3D_LinePickSensor, pickedTextureCoordinate));
				}
				break;
				case NODE_PointPickSensor:
				{
					//send points that were inside
					struct X3D_PointPickSensor *lnode = (struct X3D_PointPickSensor *)node;
					lnode->pickedPoint.n = p->stack_pointsinside->n;
					lnode->pickedPoint.p = realloc(lnode->pickedPoint.p,lnode->pickedPoint.n * 3 * sizeof(float));
					for(int ik=0;ik<p->stack_pointsinside->n;ik++){
						struct intersection_info *iinfo = vector_get_ptr(struct intersection_info,p->stack_pointsinside,ik);
						veccopy3f(lnode->pickedPoint.p[ik].c,iinfo->p);
					}
					MARK_EVENT(ptr,offsetof(struct X3D_LinePickSensor, pickedPoint));
				}
				break;
				case NODE_PrimitivePickSensor:
				//just the geometry list
				break;
				case NODE_VolumePickSensor:
				{
				}
				default:
					break;
			}

		}
		if(!ishit){
			if (node->isActive) {
				#ifdef SEVERBOSE
				printf ("transformsensor - going inactive\n");
				#endif

				node->isActive = 0;
				MARK_EVENT (ptr, offsetof(struct X3D_PrimitivePickSensor, isActive));
			}
		}

		//ask this node, and target nodes to save their modelviewmatrix for each USE, 
		//..when visited, on the upcoming frame
		for(i=0;i<unodes->n;i++)
			unodes->p[i]->_renderFlags |= VF_USE;
	} //if targets
	node->_renderFlags |= VF_USE;
}