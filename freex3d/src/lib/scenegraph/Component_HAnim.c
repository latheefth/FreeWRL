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
#include "LinearAlgebra.h"

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
FIXED for LOA-0 ie NancyDiving.x3d

Nov 5, 2016
Need to add .skin weighted vertex blending for LOA-1 ie BoxMan.x3d
2 methods:
CPU - transform coords on CPU side, try to do without re-compiling shape node as a result of change
	a) stream_polyrep actualcoord update based on tesselation indexes, for glDrawArray
	b) replace stream_polyrep with new stream_indexed so child_shape renders via glDrawElements 
		then less processing to update vertices which are kept 1:1 with original
GPU - as with CPU a) or b), except:
	jointTransforms JT[] are sent to gpu, with index and weight as vertex attributes
	https://www.opengl.org/wiki/Skeletal_Animation

How it would all work
on child_humanoid rendering call:
- clear a list of per-skinCoord-vertex indexs PSVI[] and weights PSVW[] size = skinCoord.n
- clear a flat list of transforms, [] size ~= number of joints
- clear a stack of humanoid transforms
- render skeleton
	for each Joint 
		aggregate humanoid transform like we normally do
		GPU method: 
			parse xyz and quaternion from the matrix (is that possible/easy? do I have code?)
			add {xyz,quaternion} joint transform to joint transform list JT[], get its new index JI
		CPU method
			add cumulative humanoid joint transform to joint transform list JT[], get its new index JI
		iterate over joint's list of coordIndices, and for each index
			add the jointTransformIndex JI to PSVI[index]
			add the jointWeight to go with the index PSVW[index] = wt
	for each sites or segment - render as normal scenegraph content
- render skin
	GPU method: set skeletal animation vertex blending shader flag SKELETAL on shaderflags stack
	CPU method: copy and do weighted transform of coords here using PSVI, PSVW
	for each shape in skin:
		draw as normal shape, almost, except: 
		GPU method: send (untransformed) coordinates to shader as vertices as usual
		CPU method: if SKELETAL && CPU substitute transformed coords, and send to shader
		for drawElements, send indices
		if SKELETAL && GPU, then, after compiling/fetching shader:
			send list of jointtransforms to shader as array(s) of vec4 
			send indexes to JTs to shader as vertex attributes
			send weights to shader as vertex attributes
			in shader if SKELETAL && GPU
				apply weighted transforms


*/


/* last HAnimHumanoid skinCoord and skinNormals */
//void *HANimSkinCoord = 0;
//void *HAnimSkinNormal = 0;
typedef struct pComponent_HAnim{
	struct X3D_HAnimHumanoid *HH;
	double HHMatrix[16];

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
		p->HH = NULL;

	}
}
void Component_HAnim_clear(struct tComponent_HAnim *t){
	//public
	//private
	{
		ppComponent_HAnim p = (ppComponent_HAnim)t->prv;
	}
}
//ppComponent_HAnim p = (ppComponent_HAnim)gglobal()->Component_HAnim.prv;


void compile_HAnimJoint (struct X3D_HAnimJoint *node){

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

}
void prep_HAnimJoint (struct X3D_HAnimJoint *node) {



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


void fin_HAnimJoint (struct X3D_HAnimJoint *node) {

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

void compile_HAnimSite (struct X3D_HAnimSite *node){

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

}
void prep_HAnimSite (struct X3D_HAnimSite *node) {



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


void fin_HAnimSite (struct X3D_HAnimSite *node) {

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

typedef  struct {
	double mat [16];
	float normat[9];
}  JMATRIX;
enum {
	VERTEXTRANSFORMMETHOD_CPU = 1,
	VERTEXTRANSFORMMETHOD_GPU = 2,
};
static int vertexTransformMethod = VERTEXTRANSFORMMETHOD_CPU;
void render_HAnimHumanoid (struct X3D_HAnimHumanoid *node) {
	/* save the skinCoords and skinNormals for use in following HAnimJoints */
	/* printf ("rendering HAnimHumanoid\n"); */
}

void render_HAnimJoint (struct X3D_HAnimJoint * node) {
	int i,j, jointTransformIndex;
	double modelviewMatrix[16], mvmInverse[16];
	JMATRIX jointMatrix;
	Stack *JT;
	float *PVW, *PVI;

	ppComponent_HAnim p = (ppComponent_HAnim)gglobal()->Component_HAnim.prv;
	//printf ("rendering HAnimJoint %d\n",node); 
	

	JT = p->HH->_JT;

	//step 1, generate transform
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
	matmultiplyAFFINE(jointMatrix.mat,modelviewMatrix,p->HHMatrix);
	if(p->HH->skinNormal){
		//want 'inverse-transpose' 3x3 float for transforming normals
		//(its almost the same as jointMatrix.mat except when shear due to assymetric scales)
		float fmat4[16], fmat3[9],fmat3i[9],fmat3it[9];
		matdouble2float4(fmat4,jointMatrix.mat);
		mat423f(fmat3,fmat4);
		matinverse3f(fmat3i,fmat3);
		mattranspose3f(jointMatrix.normat,fmat3i);
		//printf("jm.normat[1] %f\n",jointMatrix.normat[1]);
	}

	if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_GPU){
		//convert to quaternion + position
		//add to HH transform list
	}else if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_CPU){
		//step 2, add transform to HH transform list, get its index in list
		stack_push(JMATRIX,JT,jointMatrix);
	}
	//I'll let this index start at 1, and subtract 1 when retrieving with vector_get, 
	//so I can use jointTransformIndex==0 as a sentinal value for 'no transform stored'
	//to save me from having an extra .n transforms variable
	jointTransformIndex = vectorSize(JT); 
	
	//step 3, add transform index and weight to each skin vertex
	PVW = (float*)p->HH->_PVW;
	PVI = (float*)p->HH->_PVI;
	for(i=0;i<node->skinCoordIndex.n;i++){
		int idx = node->skinCoordIndex.p[i];
		float wt = node->skinCoordWeight.p[min(i,node->skinCoordWeight.n -1)];
		for(j=0;j<4;j++){
			if(PVI[idx*4 + j] == 0.0f){
				PVI[idx*4 +j] = (float)jointTransformIndex;
				PVW[idx*4 +j] = wt;
			}
		}
	}
	//step 4: add on any Displacer displacements
	if(p->HH->skinCoord && node->displacers.n ){
		int nsc, ndp, ni, i;
		float *psc, *pdp;
		int *ci;
		struct X3D_Coordinate *nc = (struct X3D_Coordinate*)p->HH->skinCoord;
		psc = (float*)nc->point.p;
		nsc = nc->point.n;
		for(i=0;i<node->displacers.n;i++){
			int index, j;
			float *point, weight, wdisp[3];
			struct X3D_HAnimDisplacer *dp = (struct X3D_HAnimDisplacer *)node->displacers.p[i];
				
			weight = dp->weight;
			//printf(" %f ",weight);
			pdp = (float*)dp->displacements.p;
			ndp = dp->displacements.n;

			ni = dp->coordIndex.n;
			ci = dp->coordIndex.p;
			for(j=0;j<ni;j++){
				index = ci[j];
				point = &psc[index*3];
				vecscale3f(wdisp,&pdp[j*3],weight);
				vecadd3f(point,point,wdisp);
			}
		}
		if(0){ //this is done in child_HAnimHumanoid for the skinCoord parents
			//force HAnimSegment.children[] shape nodes using segment->coord to recompile
			int k;
			p->HH->skinCoord->_change++;
			Stack *parents = p->HH->skinCoord->_parentVector;
			for(k=0;k<vectorSize(parents);k++){
				struct X3D_Node *parent = vector_get(struct X3D_Node*,parents,k);
				parent->_change++;
			}
		}

	}

}
int vecsametol3f(float *a, float *b, float tol){
	int i,isame = TRUE;
	for(i=0;i<3;i++)
		if(fabsf(a[i] - b[i]) > tol) isame = FALSE;
	return isame;
}
void compile_HAnimHumanoid(struct X3D_HAnimHumanoid *node){
	//printf("compile_HAnimHumanoid\n");
	//check if the coordinate count is the same
	int nsc = 0, nsn = 0;
	float *psc = NULL, *psn = NULL;
	if(node->skinCoord && node->skinCoord->_nodeType == NODE_Coordinate){
		struct X3D_Coordinate * nc = (struct X3D_Coordinate * )node->skinCoord;
		nsc = nc->point.n;
		psc = (float*)nc->point.p;
		node->_origCoords = realloc(node->_origCoords,nsc*3*sizeof(float));
		memcpy(node->_origCoords,psc,nsc*3*sizeof(float));
		if(0){
			//find a few coordinates in skinCoord I hacked, by xyz, and give me their index, for making a displacer
			float myfind[9] = {-0.030000, -0.070000, 1.777000 ,  -0.070000, 1.777000, 0.130000 ,  1.777000, 0.130000, 0.070000 };
			for(int i=0;i<nsc;i++){
				for(int j=0;j<3;j++)
					if(vecsametol3f(&psc[i*3],&myfind[j*3],.001f)){
						printf("%d %f %f %f\n",i,myfind[j*3 + 0],myfind[j*3 +1],myfind[j*3 +2]);
					}
			}
		}
	}
	if(node->skinNormal && node->skinNormal->_nodeType == NODE_Normal){
		struct X3D_Normal * nn = (struct X3D_Normal * )node->skinNormal;
		//Assuming 1 normal per coord, coord 1:1 normal
		nsn = nn->vector.n;
		psn = (float*)nn->vector.p;
		node->_origNorms = realloc(node->_origNorms,nsn*3*sizeof(float));
		memcpy(node->_origNorms,psn,nsn*3*sizeof(float));
	}
	
	//allocate the joint-transform_index and joint-weight arrays
	//Nov 2016: max 4: meaning each skinCoord can have up to 4 joints referencing/influencing it
	//4 chosen so it's easier to port to GPU method with vec4
	if(node->_NV == 0 || node->_NV != nsc){
		node->_PVI = realloc(node->_PVI,nsc*4*sizeof(float)); //indexes, up to 4 joints per skinCoord
		node->_PVW = realloc(node->_PVW,nsc*4*sizeof(float)); //weights, up to 4 joints per skinCoord
		node->_NV = nsc;
	}
	//allocate the transform array
	if(node->_JT == NULL)
		if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_GPU){
			//new stack quat + position
		}else if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_CPU){
			node->_JT = newStack(JMATRIX); //we don't know how many joints there are - need to count as we go
		}
	MARK_NODE_COMPILED
}

void child_HAnimHumanoid(struct X3D_HAnimHumanoid *node) {
	int nc;
	float *originalCoords;
	Stack *JT;
	ppComponent_HAnim p = (ppComponent_HAnim)gglobal()->Component_HAnim.prv;
	COMPILE_IF_REQUIRED

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
	memset(node->_PVI,0,4*node->_NV*sizeof(float));
	memset(node->_PVW,0,4*node->_NV*sizeof(float));
	JT = node->_JT; 
	JT->n = 0;

	//in theory, HH, HHMatrix could be a stack, so you could have an hanimhumaoid within an hanimhunaniod
	p->HH = node;
	{
		double modelviewMatrix[16];
		FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelviewMatrix);
		matinverseAFFINE(p->HHMatrix,modelviewMatrix);
	}
	if(node->skin.n){
		if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_CPU){
			//save original coordinates before rendering skeleton
			// - HAnimJoint may have displacers that change the Coords
			//transform each vertex and its normal using weighted transform
			int i,j,nsc = 0, nsn = 0;
			float *psc = NULL, *psn = NULL;
			if(node->skinCoord && node->skinCoord->_nodeType == NODE_Coordinate){
				struct X3D_Coordinate * nc = (struct X3D_Coordinate * )node->skinCoord;
				struct X3D_Normal *nn = (struct X3D_Normal *)node->skinNormal; //might be NULL 
				nsc = nc->point.n;
				psc = (float*)nc->point.p;
				memcpy(psc,node->_origCoords,3*nsc*sizeof(float));
				if(nn){
					nsn = nn->vector.n;
					psn = (float *)nn->vector.p;
					memcpy(psn,node->_origNorms,3*nsn*sizeof(float));
				}
			}
		}
	}
	if(1) normalChildren(node->skeleton);

	if(node->skin.n){
		if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_CPU){
			//save original coordinates
			//transform each vertex and its normal using weighted transform
			int i,j,nsc = 0, nsn = 0;
			float *psc = NULL, *psn = NULL;
			if(node->skinCoord && node->skinCoord->_nodeType == NODE_Coordinate){
				struct X3D_Coordinate * nc = (struct X3D_Coordinate * )node->skinCoord;
				struct X3D_Normal *nn = (struct X3D_Normal *)node->skinNormal; //might be NULL 
				nsc = nc->point.n;
				psc = (float*)nc->point.p;
				//memcpy(psc,node->_origCoords,3*nsc*sizeof(float));
				if(nn){
					nsn = nn->vector.n;
					psn = (float *)nn->vector.p;
					//memcpy(psn,node->_origNorms,3*nsn*sizeof(float));
				}

				for(i=0;i<nsc;i++){
					float totalWeight;
					float *point, *norm; 
					float newpoint[3], newnorm[3];
					float *PVW, *PVI;

					point = &psc[i*3];
					norm = NULL;
					if(nn) norm = &psn[i*3];
					PVW = node->_PVW;
					PVI = node->_PVI;

					memset(newpoint,0,3*sizeof(float));
					memset(newnorm,0,3*sizeof(float));
					totalWeight = 0.0f;
					for(j=0;j<4;j++){
						int jointTransformIndex = (int)PVI[i*4 + j];
						float wt = PVW[i*4 + j];
						if(jointTransformIndex > 0){
							float tpoint[3], tnorm[3];
							JMATRIX jointMatrix;
							jointMatrix = vector_get(JMATRIX,node->_JT,jointTransformIndex -1);
							transformf(tpoint,point,jointMatrix.mat);
							vecscale3f(tpoint,tpoint,wt);
							vecadd3f(newpoint,newpoint,tpoint);
							if(nn){
								transform3x3f(tnorm,norm,jointMatrix.normat);
								vecnormalize3f(tnorm,tnorm); 
								vecscale3f(tnorm,tnorm,wt);
								vecadd3f(newnorm,newnorm,tnorm);
							}
							totalWeight += wt;
						}
					}
					if(totalWeight > 0.0f){
						vecscale3f(newpoint,newpoint,1.0f/totalWeight);
						veccopy3f(point,newpoint);
						if(nn){
							vecscale3f(newnorm,newnorm,1.0f/totalWeight);
							vecnormalize3f(norm,newnorm);
						}
					}
				}
				if(0){
					//print out before and after coords
					float *osc = node->_origCoords;
					for(i=0;i<nsc;i++){
						printf("%d ",i);
						for(j=0;j<3;j++) printf("%f ",psc[i*3 +j]);
						printf("/ ");
						for(j=0;j<3;j++) printf("%f ",osc[i*3 +j]);
						printf("\n");
					}
					printf("\n");
				}
				//trigger recompile of skin->shapes when rendering skin
				//Nov 6, 2016: recompiling a shape / polyrep on each frame eats memory 
				//NODE_NEEDS_COMPILING
				if(1){
					int k;
					node->skinCoord->_change++;
					Stack *parents = node->skinCoord->_parentVector;
					for(k=0;k<vectorSize(parents);k++){
						struct X3D_Node *parent = vector_get(struct X3D_Node*,parents,k);
						parent->_change++;
					}
				}

			}
		}else if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_GPU){
			//push shader flaga with += SKELETAL
		}

		if(1) normalChildren(node->skin);
		if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_GPU){
			//pop shader flags
		} else if(vertexTransformMethod == VERTEXTRANSFORMMETHOD_CPU){
			//restore original coordinates 
			int nsc, nsn;
			float *psc, *psn;
			struct X3D_Coordinate * nc = (struct X3D_Coordinate * )node->skinCoord;
			struct X3D_Normal * nn = (struct X3D_Normal * )node->skinNormal;
			nsc = nc->point.n;
			psc = (float*)nc->point.p;
			memcpy(psc,node->_origCoords,3*nsc*sizeof(float));
			if(nn){
				nsn = nn->vector.n;
				psn = (float*)nn->vector.p;
				memcpy(psn,node->_origNorms,3*nsn*sizeof(float));
			}
		}
	} //if skin
	fin_sibAffectors((struct X3D_Node*)node,&node->__sibAffectors);


	/* did we have that directionalLight? */
	//LOCAL_LIGHT_OFF
}


void child_HAnimJoint(struct X3D_HAnimJoint *node) {

	//CHILDREN_COUNT
	/* any children at all? */
	//if (nc==0) return;

	/* should we go down here? */
	//RETURN_FROM_CHILD_IF_NOT_FOR_ME

	/* do we have to sort this node? */

	/* just render the non-directionalLight children */
	normalChildren(node->children);


}
float *vecmix3f(float *out3, float* a3, float *b3, float fraction){
	int i;
	for(i=0;i<3;i++){
		out3[i] = (1.0f - fraction)*a3[i] + fraction*b3[i];
	}
	return out3;
}
void child_HAnimSegment(struct X3D_HAnimSegment *node) {

	//CHILDREN_COUNT


//note to implementer: have to POSSIBLE_PROTO_EXPANSION(node->coord, tmpN)

	/* any children at all? */
	//if (nc==0) return;

	/* should we go down here? */
	//RETURN_FROM_CHILD_IF_NOT_FOR_ME

	/* do we have to sort this node? Only if not a proto - only first node has visible children. */

	/* now, just render the non-directionalLight children */
	if(node->coord && node->displacers.n){

		int nsc, ndp, ni, i;
		float *psc, *pdp;
		int *ci;
		struct X3D_Coordinate *nc = (struct X3D_Coordinate*)node->coord;
		psc = (float*)nc->point.p;
		nsc = nc->point.n;
		if(!node->_origCoords)
			node->_origCoords = malloc(3*nsc*sizeof(float));
		memcpy(node->_origCoords,psc,3*nsc*sizeof(float));
		for(i=0;i<node->displacers.n;i++){
			int index, j;
			float *point, weight, wdisp[3];
			struct X3D_HAnimDisplacer *dp = (struct X3D_HAnimDisplacer *)node->displacers.p[i];
				
			weight = dp->weight;
			//printf(" %f ",weight);
			pdp = (float*)dp->displacements.p;
			ndp = dp->displacements.n;

			ni = dp->coordIndex.n;
			ci = dp->coordIndex.p;
			for(j=0;j<ni;j++){
				index = ci[j];
				point = &psc[index*3];
				vecscale3f(wdisp,&pdp[j*3],weight);
				vecadd3f(point,point,wdisp);
			}
		}
		if(1){
			//force HAnimSegment.children[] shape nodes using segment->coord to recompile
			int k;
			node->coord->_change++;
			Stack *parents = node->coord->_parentVector;
			for(k=0;k<vectorSize(parents);k++){
				struct X3D_Node *parent = vector_get(struct X3D_Node*,parents,k);
				parent->_change++;
			}
		}

		if(0){
			//find a few coordinates in segment->coord I hacked, by xyz, and give me their index, 
			// for making a displacer
			float myfind[9] = {-0.029100, 1.603000, 0.042740,    -0.045570, 1.601000, 0.036520,    -0.018560, 1.600000, 0.043490 };
			int found = FALSE;
			printf("\n");
			for(i=0;i<nsc;i++){
				for(int j=0;j<3;j++)
					if(vecsametol3f(&psc[i*3],&myfind[j*3],.0001f)){
						printf("%d %f %f %f\n",i,myfind[j*3 + 0],myfind[j*3 +1],myfind[j*3 +2]);
						found = TRUE;
					}
			}
			if(found)
				printf("\n");
		}
	}
	normalChildren(node->children);
	if(node->coord && node->displacers.n){
		int nsc;
		float *psc;
		struct X3D_Coordinate *nc = (struct X3D_Coordinate*)node->coord;
		psc = (float*)nc->point.p;
		nsc = nc->point.n;
		memcpy(psc,node->_origCoords,3*nsc*sizeof(float));
	}
}


void child_HAnimSite(struct X3D_HAnimSite *node) {

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


}

