/*


X3D H-Anim Component

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
#include "../opengl/Material.h"
#include "../opengl/OpenGL_Utils.h"
#include "Children.h"
#include "../scenegraph/RenderFuncs.h"
#include "../opengl/Frustum.h"

/* #include "OpenFW_GL_Utils.h" */

/*
HAnim examples:
http://www.web3d.org/x3d/content/examples/Basic/HumanoidAnimation/
								Octaga				InstantReality
- BoxMan.x3d					good				doesn't animate
- AllenDutton.x3d				skin stuck			good
- NancyStandShootRifleM24.x3d	good				anim good, skin bad
- (KelpForest) NancyDiving.x3d  good				good
HAnim Prototypes:
http://www.web3d.org/x3d/content/examples/Basic/HumanoidAnimation/_pages/page11.html
HAnim examples:
http://www.web3d.org/x3d/content/examples/Basic/#HumanoidAnimation
Specs:
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/hanim.html
These specs don't articulate the field meanings, instead point to an ISO doc:
http://www.iso.org/iso/home/store/catalogue_tc/catalogue_detail.htm?csnumber=33912
I19774 about $80
Here's some free online docs:
http://www.h-anim.org/
http://h-anim.org/Specifications/H-Anim200x/ISO_IEC_FCD_19774/
http://h-anim.org/Specifications/H-Anim200x/ISO_IEC_FCD_19774/ObjectInterfaces.html
- Humanoid	has transform, skeleton, skin, (flat lists of) skinCoord, skinNormal, segments,joints,sites,viewpoints
- Joint		has transform, children, displacers, skinCoord indx,wt
- Segment	has            children, displacers, mass, coord
- Site		has transform, children 
- Displacer has                      displacements, coord, wt, coord index, 
All the fields are discussed.

July 2016 
- where we left off years ago?
- define HANIMHANIM below to compile - lots of errors
Nov 2016
- stack errors fixed by removing return; at top of some functions
x but doesn't render/animate correctly: whole body frozen, various limbs moving independently/dismembered

Related Links on HAnim
http://www.web3d.org/working-groups/humanoid-animation-h-anim
- The tutorial
Links on character animation for real-time graphics:
http://www.cescg.org/CESCG-2002/MPutzKHufnagl/paper.pdf
- Character Animation for Real-time Applications
http://apc.dcti.iscte.pt/praticas/Real-Time%20Character%20Animation%20for%20Computer%20Games.pdf
- (Anderson) Real-Time Character Animation for Computer Games
https://books.google.ca/books?id=O0sxtDeT5PEC&pg=PA125&lpg=PA125&dq=Animating+single+mesh+character+in+real+time&source=bl&ots=u8gETEoSM0&sig=AW7nQB25K4IxjZp_7xSlLJRxiCQ&hl=en&sa=X&ved=0ahUKEwiQ6NHdoarOAhVX2mMKHVrdDR8Q6AEIJjAB#v=onepage&q=Animating%20single%20mesh%20character%20in%20real%20time&f=false
- book: Real-time Character Animation with Visual C++ 
- shows additive approach to vertex blending:
	vertex {
		xyz original;
		xyz world;
		int number_of_limbs_influencing;
	}
	set number_of_limbs influencing vertex;
	on each frame, 
		zero worlds
		Iterate over limbs, transforming originals influenced and adding/summing onto worlds.
		divide each world by number_of_limbs
http://www.nvidia.com/object/doc_characters.html
- Nvidia link page for various game programming algos with gpu acceleration
http://ruh.li/AnimationVertexSkinning.html
- Skinning shader
http://http.developer.nvidia.com/GPUGems/gpugems_ch04.html
- Bind pose, dawn
http://www.3dgep.com/gpu-skinning-of-md5-models-in-opengl-and-cg/
- Glsl impl of skinning
http://tech-artists.org/wiki/Vertex_Skinning
- ogre sample shader
https://www.cs.tcd.ie/publications/tech-reports/reports.06/TCD-CS-2006-46.pdf
- dual quat skinning


Q if the skeleton isnt rendered, when would you traverse the skeleton
And whats the output?
Q isnt there some kind of ik solver that could/should be exposed to sai as a node?
Otherwise you will need compiled/fast scrpt engine if its all in js, and
scene authors will need to know all that stuff.
https://en.wikipedia.org/wiki/Inverse_kinematics
https://en.wikipedia.org/wiki/Moment_of_inertia
dug9 july 2016: What's Weird about HAnim: 
a) looks like there's very little animation work we have to do in our browser
	the scene author has to do it all in js / via SAI
b) the skeleton isn't rendered (if no geom on sites/segments?)
	so when do you traverse the joints and what's the output?

dug9 aug 2016: would this work: 
Just render_HAnimHumanoid (as un-shared/opaque private scenegraph?)
- traverse the joints and segments privately from HanimHumanoid to call their render_ functions
- no HanimHumanoid? then don't render any joints or segments - don't list virtual functions for them

Design Options:
1. 2-step
a) interpolate the pose of joints and segments relative to HanimHumanoid
	i) start with Identity transform at HanimHumanoid
	ii) traverse down segments and joints, and sites, pushing, multiplying and saving 
		the cumulative transform in each segment/joint/site
b) for each segment/joint/site (done at the HanimHumanoid level)
	i) push cumulative pose for segment/joint/site onto transform stack
	ii) render any attached geometry via noralchildren
2. Combined step
a) traverse down segments pushing and multiplying pose
b) when visiting a segment/joint/site render its children geometry
	x this wont work with skin/'skinning', just attached solid geometry ie scuba tank
	* so instead of the 'render' step for skin, there would be vertex-update step
3. Best Guess
a) traverse skeleton joints rendering attached solid geometry, and updating influenced skin vertices
b) divide skin vertices by number of influencers
c) render skin

Requirements:
- single deformable mesh should be 'easy' / possible / efficient to update / interpolate 

How to update single deformable mesh? 
Please research, as its a common thing in game programming.
Guesses:
- do we have/could we have a system for interpolating the compiled polyrep?
- for example if during compiling the polyrep we kept an expanded index, 
  so a vertex index stored in a joint could find all the triangle vertices in the compiled / tesselated / triangulated mesh?
	array[original index] [list of compiled polyrep vertex indexes]
- or when compiling the HAnimHumanoid, would/could we break the polyrep into chunks associated with segments
	based on 3D proximity
- or would we transform the whole mesh for each segment, except weight the points differently,
	so that the final mesh is a per-vertex-weighted sum of all segment meshes

Nov 2016
state Nov 3, 2016: 
NancyDiving.x3d (scene :HanimHumanoid(HH) with 
	HH->Skeleton children[ RootJoint, Joints, Segments] and 
	HH->joints, HH->segments
	no HH->skin,->viewpoints
x I don't see a single skin mesh/vertices being updated by weighted transforms
x I don't see any mention of Displacer node type
H: its a LOA 0 (or lowest level, with separate segments for each limb)

freewrl 
x I don't see a single skin mesh vertices being updated by weighted transforms in code below
x I don't see any mention of Displacer node type below, although its in perl/structs.h
H: 
it was put together quickly using boilerplate scenegraph calls, for LOA 0,
but without detailed custom code or testing to make it work for LOA 1+

freewrl rendering of NancyDiving.x3d
x skin frozen, while indvidual body segments are transformed separately / dismembered, 
x seem to be missing rotations on the segments


*/

#define HANIMHANIM 1
static int animate_site = 0;
static int animate_joint = 1;
/* last HAnimHumanoid skinCoord and skinNormals */
//void *HANimSkinCoord = 0;
//void *HAnimSkinNormal = 0;
typedef struct pComponent_HAnim{
	void *HANimSkinCoord;// = 0;
	void *HAnimSkinNormal;// = 0;

}* ppComponent_HAnim;
void *Component_HAnim_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_HAnim));
	memset(v,0,sizeof(struct pComponent_HAnim));
	return v;
}
void Component_HAnim_init(struct tComponent_HAnim *t){
	//public
	//private
	t->prv = Component_HAnim_constructor();
	{
		ppComponent_HAnim p = (ppComponent_HAnim)t->prv;
		p->HANimSkinCoord = 0;
		p->HAnimSkinNormal = 0;

	}
}
//ppComponent_HAnim p = (ppComponent_HAnim)gglobal()->Component_HAnim.prv;


void compile_HAnimJoint (struct X3D_HAnimJoint *node){
#ifdef HANIMHANIM
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

	//REINITIALIZE_SORTED_NODES_FIELD(node->children,node->_sortedChildren);
	MARK_NODE_COMPILED
#endif
}
void prep_HAnimJoint (struct X3D_HAnimJoint *node) {

#ifdef HANIMHANIM
	if(animate_joint) {

		COMPILE_IF_REQUIRED

		/* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
			* so we do nothing here in that case -ncoder */

		/* printf ("prep_Transform, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
		render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

		/* do we have any geometry visible, and are we doing anything with geometry? */
		//OCCLUSIONTEST

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
#endif
}


void fin_HAnimJoint (struct X3D_HAnimJoint *node) {
#ifdef HANIMHANIM
	if(animate_joint){
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
#endif
} 

void compile_HAnimSite (struct X3D_HAnimSite *node){
#ifdef HANIMHANIM
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

	//REINITIALIZE_SORTED_NODES_FIELD(node->children,node->_sortedChildren);
	MARK_NODE_COMPILED
#endif
}
void prep_HAnimSite (struct X3D_HAnimSite *node) {

#ifdef HANIMHANIM
	if(animate_joint) {

		COMPILE_IF_REQUIRED

		/* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
			* so we do nothing here in that case -ncoder */

		/* printf ("prep_Transform, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
		render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

		/* do we have any geometry visible, and are we doing anything with geometry? */
		//OCCLUSIONTEST

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
#endif
}


void fin_HAnimSite (struct X3D_HAnimSite *node) {
#ifdef HANIMHANIM
	if(animate_joint){
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
#endif
} 


void render_HAnimHumanoid (struct X3D_HAnimHumanoid *node) {
	/* save the skinCoords and skinNormals for use in following HAnimJoints */
	/* printf ("rendering HAnimHumanoid\n"); */
}

void render_HAnimJoint (struct X3D_HAnimJoint * node) {
return;
	/* printf ("rendering HAnimJoint %d\n",node); */

}

void child_HAnimHumanoid(struct X3D_HAnimHumanoid *node) {
	int nc;
	//LOCAL_LIGHT_SAVE

	/* any segments at all? */
/*
printf ("hanimHumanoid, segment coutns %d %d %d %d %d %d\n",
		node->joints.n,
		node->segments.n,
		node->sites.n,
		node->skeleton.n,
		node->skin.n,
		node->viewpoints.n);
*/

	nc = node->joints.n + node->segments.n + node->viewpoints.n + node->sites.n +
		node->skeleton.n + node->skin.n;

	RETURN_FROM_CHILD_IF_NOT_FOR_ME 

	if(renderstate()->render_vp){
		/* Lets do viewpoints */
		normalChildren(node->viewpoints);
		return;
	}


	// segments, joints, sites are flat-lists for convenience
	// skeleton is the scenegraph-like transform hierarchy of joints and segments and sites
	// skin relies on something updating its vertices based on skeleton transforms
	/* Lets do segments first */
	/* now, just render the non-directionalLight segments */
	if(0) normalChildren(node->segments);


	/* Lets do joints second */
	/* do we have to sort this node? */
	/* now, just render the non-directionalLight joints */
	if(0) normalChildren(node->joints);


	/* Lets do sites third */
	/* do we have to sort this node? */
	/* do we have a local light for a child? */
	//LOCAL_LIGHT_CHILDREN(node->sites);
	/* now, just render the non-directionalLight sites */
	if(0) normalChildren(node->sites);

	prep_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);
	/* Lets do skeleton fourth */
	/* do we have to sort this node? */
	/* now, just render the non-directionalLight skeleton */
	//skeleton is the basic thing to render for LOA 0
	if(1) normalChildren(node->skeleton);

	/* Lets do skin fifth */
	/* do we have to sort this node? */
	/* do we have a local light for a child? */
	//LOCAL_LIGHT_CHILDREN(node->skin);

	/* now, just render the non-directionalLight skin */
	// dug9 Aug 2016: I think its just the skin that would/should get lights
	// because the other ones -site, segment etc- will have their own children field 
	// and can put its local lights there, but skin can't
	if(0) normalChildren(node->skin);
	fin_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);


	/* did we have that directionalLight? */
	//LOCAL_LIGHT_OFF
}


void child_HAnimJoint(struct X3D_HAnimJoint *node) {
#ifdef HANIMHANIM
	//CHILDREN_COUNT
	/* any children at all? */
	//if (nc==0) return;

	/* should we go down here? */
	//RETURN_FROM_CHILD_IF_NOT_FOR_ME

	/* do we have to sort this node? */

	/* just render the non-directionalLight children */
	normalChildren(node->children);

#endif
}

void child_HAnimSegment(struct X3D_HAnimSegment *node) {
#ifdef HANIMHANIM
	//CHILDREN_COUNT


//note to implementer: have to POSSIBLE_PROTO_EXPANSION(node->coord, tmpN)

	/* any children at all? */
	//if (nc==0) return;

	/* should we go down here? */
	//RETURN_FROM_CHILD_IF_NOT_FOR_ME

	/* do we have to sort this node? Only if not a proto - only first node has visible children. */

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);
#endif
}


void child_HAnimSite(struct X3D_HAnimSite *node) {
#ifdef HANIMHANIM
	//CHILDREN_COUNT
	//LOCAL_LIGHT_SAVE
	//RETURN_FROM_CHILD_IF_NOT_FOR_ME

	/* do we have to sort this node? */

	/* do we have a local light for a child? */
	//LOCAL_LIGHT_CHILDREN(node->children);
	prep_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);

	//LOCAL_LIGHT_OFF
	fin_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);

#endif
}

