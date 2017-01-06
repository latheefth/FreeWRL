/*


X3D Rigid Body Physics Component

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
//#include "Component_RigidBodyPhysics.h"
#include "Children.h"


typedef struct pComponent_RigidBodyPhysics{
	int something;
}* ppComponent_RigidBodyPhysics;
void *Component_RigidBodyPhysics_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_RigidBodyPhysics));
	memset(v,0,sizeof(struct pComponent_RigidBodyPhysics));
	return v;
}
void Component_RigidBodyPhysics_init(struct tComponent_RigidBodyPhysics *t){
	//public
	//private
	t->prv = Component_RigidBodyPhysics_constructor();
	{
		ppComponent_RigidBodyPhysics p = (ppComponent_RigidBodyPhysics)t->prv;
		p->something = 0;
	}
}
void Component_RigidBodyPhysics_clear(struct tComponent_RigidBodyPhysics *t){
	//public
}

//ppComponent_RigidBodyPhysics p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;


//Q. do you love criptic macros? Here's a few:
int NNC0(struct X3D_Node* node){
	//NNC Node Needs Compiling
	return NODE_NEEDS_COMPILING;
	//return FALSE;
}
void MNC0(struct X3D_Node* node){
	//MNC Mark Node Compiled
	MARK_NODE_COMPILED;
}
void MNX0(struct X3D_Node* node){
	//MNX Mark Node Changed
	node->_change++;
}
#define NNC(A) NNC0(X3D_NODE(A))
#define MNC(A) MNC0(X3D_NODE(A))
#define MNX(A) MNX0(X3D_NODE(A))
#define PPX(A) getTypeNode(X3D_NODE(A)) //possible proto expansion


void rbp_run_physics();
void set_physics();

//#undef WITH_RBP
#ifdef WITH_RBP
//START MIT LIC >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//#define dSINGLE 1  //Q. do we need to match the physics lib build
#define dDOUBLE 1
#include <ode/ode.h>

/*
Physics options considered:
0. PAL - physics abstraction layer (in c++, not maintained)
1. bullet - very popular, c++/no flat-c interface, large
2. ode - c api (even though implemented in c++), 
	- maps well to x3d, specs say 'implemented by ode' in one place
decision feb 2016: ode

X3D
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/rigid_physics.html

ODE
http://ode-wiki.org/wiki/index.php?title=Manual:_Rigid_Body_Functions
- body, world
http://ode-wiki.org/wiki/index.php?title=Manual:_Collision_Detection
- geoms, geom postion, geom class, collide(), spaces, contact points

X3D					ODE	
CollidableShape		geoms	
CollidableOffset	GeomTransform T ?
	- both: GeomSetPosition,Rotation,Quat,
CollisionCollection	Geom Class / category bits	
CollisionSensor		dSpaceCollide() collide2()	callback()
CollisionSpace		spaces	
Contact				Contact points	
RigidBody			body	
RigidBodyCollection	world	

Initializing physics nodes:
Freewrl has a few ways of initializing a node:
A. during parsing > createX3DNode(), calling a add_ function, like our add_physics()
	-done before field values are set, so just good for registering existence, not initializing state
B. during a render_hier() pass, when 'visiting' a node, detecting if it isn't initialized, or changed, and compiling
	-advantage: complex node fields will be populated, parents and children will be accessible, modelview transform known
C. in startofloopnodeupdates() done once per frame
D. after event processing in execution model ie our rbp_run_physics()

Of the physics nodes, only CollidableShape and its extra-transform wrapper CollidableOffset need
to be visited on a render_hier pass for visual rendering.

X3D declared Node		Init	Field value node types
RigidBodyCollection		A,D		RigidBody
RigidBody				  D		CollidableShape, CollidableOffset
CollisionCollection		  D		CollidableShape, CollidableOffset, CollisionSpace
CollisionSpace			  D		CollidableShape, CollidableOffset, CollisionSpace
CollidableShape			B,D		Shape
CollidableOffset		B,D		CollidableShape
CollisionSensor			A,D		CollisionCollection

Generated Node			Generating Node
CollisionCollection		RigidBodyCollection
Contact					CollisionSensor


*/

//some crap copied & pasted by dug9 from ode's demo_boxstack.cpp demo program:
// some constants

#define NUM 100			// max number of objects
#define DENSITY (1.0)		// density of all objects
#define GPB 3			// maximum number of geometries per body
#define MAX_CONTACTS 8          // maximum number of contact points per body
#define MAX_FEEDBACKNUM 20
#define GRAVITY         REAL(0.5)
#define USE_GEOM_OFFSET 1

// dynamics and collision objects

typedef struct MyObject {
  dBodyID body;			// the body
  dGeomID geom[GPB];		// geometries representing this body
} MyObject;

static int num=0;		// number of objects in simulation
static int nextobj=0;		// next object to recycle if num==NUM
static dWorldID world = NULL;
static dSpaceID space = NULL;;
static MyObject obj[NUM];
static dJointGroupID contactgroup;  //used by nearCallback for per-frame flushable contact joints
static int selected = -1;	// selected object
static int show_aabb = 0;	// show geom AABBs?
static int show_contacts = 0;	// show contact points?
static int random_pos = 1;	// drop objects from random position?
static int write_world = 0;
static int show_body = 0;

typedef struct MyFeedback {
  dJointFeedback fb;
  bool first;
}MyFeedback;
static int doFeedback=0;
static MyFeedback feedbacks[MAX_FEEDBACKNUM];
static int fbnum=0;


//our registered lists (should be in gglobal p-> or scene->)
static struct Vector *x3dworlds = NULL;
static struct Vector *x3dcollisionsensors = NULL;
static struct Vector *x3dcollisioncollections = NULL; //


//I'm not sure what I'll be setting as *data on geom
//here's a method to use only one pointer can be either sensor or collection
struct X3D_CollidableShape * getCollidableShapeFromData(void *cdata){
	struct X3D_CollidableShape * xshape = NULL;
	if(cdata)
		xshape = (struct X3D_CollidableShape*)cdata;
	return xshape;
}
struct X3D_CollisionSensor * getCollisionSensorFromCsensor(void *csensor){
	struct X3D_CollisionSensor *csens = NULL;
	if(csensor){
		switch(X3D_NODE(csensor)->_nodeType){
			case NODE_CollisionSensor:
				csens = (struct X3D_CollisionSensor*)csensor;
			break;
			case NODE_CollisionCollection:
				{
					struct X3D_CollisionCollection *ccol = (struct X3D_CollisionCollection*)csensor;
					if(ccol->_csensor)
						csens = (struct X3D_CollisionSensor*)ccol->_csensor;
				}
			default:
			break;
		}
		if(csens && !csens->enabled) csens = NULL;
	}
	return csens;
}
struct X3D_CollisionCollection * getCollisionCollectionFromCsensor(void *csensor){
	struct X3D_CollisionCollection *ccol = NULL;
	if(csensor){
		switch(X3D_NODE(csensor)->_nodeType){
			case NODE_CollisionSensor:
				{
					struct X3D_CollisionSensor* csens = (struct X3D_CollisionSensor*)csensor;
					if(csens->collider)
						ccol =	(struct X3D_CollisionCollection*)csens->collider;
				}
			break;
			case NODE_CollisionCollection:
				ccol =	(struct X3D_CollisionCollection*)csensor;
			break;
			default:
			break;
		}
		if(ccol && !ccol->enabled) ccol = NULL;
	}
	return ccol;
}
static struct X3D_Contact static_contacts_p[100];
static int static_contacts_n = 0;
static struct X3D_Contact *static_contacts_initializer = NULL;
static int static_contacts_initialized = FALSE;
// this is called by dSpaceCollide when two objects in space are
// potentially colliding.
// http://ode.org/ode-latest-userguide.html#sec_10_5_0

static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
    if (dGeomIsSpace (o1) || dGeomIsSpace (o2)) {
      // colliding a space with something
      dSpaceCollide2 (o1,o2,data,&nearCallback);
      // collide all geoms internal to the space(s)
      if (dGeomIsSpace (o1)) {
		struct X3D_CollisionSpace *cspace = dGeomGetData(o1);
		if(cspace->enabled)
			dSpaceCollide ((dSpaceID)o1,data,&nearCallback);
	  }
      if (dGeomIsSpace (o2)) {
		struct X3D_CollisionSpace *cspace = dGeomGetData(o2);
		if(cspace->enabled)
			dSpaceCollide ((dSpaceID)o2,data,&nearCallback);
	  }
    } else {
		int i, numc;
		void *cdata1, *cdata2;
		struct X3D_CollidableShape *xshape1, *xshape2;
		//struct X3D_RigidBody *xbody1, *xbody2;
		struct X3D_CollisionSensor *xsens1, *xsens2;
		struct X3D_CollisionCollection *xcol1, *xcol2, *xcol;
		static int count = 0;

		dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per box-box
		// if (o1->body && o2->body) return;

		// exit without doing anything if the two bodies are connected by a joint
		dBodyID b1 = dGeomGetBody(o1);
		dBodyID b2 = dGeomGetBody(o2);
		if (b1 && b2 && dAreConnectedExcluding (b1,b2,dJointTypeContact)) return;


		//X3D Component_RigidBodyPhysics
		//somewhere we need to spit out X3DContacts if there was a X3DCollisionSensor
		cdata1 = dGeomGetData(o1);
		cdata2 = dGeomGetData(o2);
		xshape1 = getCollidableShapeFromData(cdata1);
		xshape2 = getCollidableShapeFromData(cdata2);
		xcol1 = getCollisionCollectionFromCsensor(xshape1->_csensor);
		xcol2 = getCollisionCollectionFromCsensor(xshape2->_csensor);
		xsens1 = getCollisionSensorFromCsensor(xshape1->_csensor);
		xsens2 = getCollisionSensorFromCsensor(xshape2->_csensor);
		if(0) if(count < 20){
			if(xsens1) printf("have csens1 %x\n",xsens1);
			if(xsens2) printf("have csens2 %x\n",xsens2);
			if(xcol1) printf("have ccol1 %x\n",xcol1);
			if(xcol2) printf("have ccol2 %x\n",xcol2);
		}
		if(xcol1 && !xcol1->enabled ) return;
		if(xcol2 && !xcol2->enabled ) return;
		if(xshape1 && !xshape1->enabled) return;
		if(xshape2 && !xshape2->enabled) return;

		count++;
		xcol = xcol1 ? xcol1 : xcol2; //do we only need one, or how to pick which one?

		//prep some defaults for any contacts found in dCollide
		// http://ode.org/ode-latest-userguide.html#sec_7_3_7
		for (i=0; i<MAX_CONTACTS; i++) {
			//defaults
			contact[i].surface.mode = dContactBounce; // | dContactSoftCFM;
			contact[i].surface.mu = .1; //dInfinity;
			contact[i].surface.mu2 = 0;
			contact[i].surface.bounce = .2;
			contact[i].surface.bounce_vel = 0.1;
			contact[i].surface.soft_cfm = 0.01;
			
			if(xcol){
				contact[i].surface.mode = xcol->_appliedParametersMask; //dContactBounce; // | dContactSoftCFM;
				contact[i].surface.mu = xcol->slipFactors.c[0]; //dInfinity;
				contact[i].surface.mu2 = xcol->slipFactors.c[1];
				contact[i].surface.bounce = xcol->bounce;
				contact[i].surface.bounce_vel = xcol->minBounceSpeed;
				contact[i].surface.soft_cfm = xcol->softnessConstantForceMix;
				contact[i].surface.soft_erp = xcol->softnessErrorCorrection;
				contact[i].surface.motion1 = xcol->surfaceSpeed.c[0];
				contact[i].surface.motion2 = xcol->surfaceSpeed.c[1];
			}
		}



		if (numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom,sizeof(dContact)))
		{
			const dReal ss[3] = {0.02,0.02,0.02};
			dMatrix3 RI;
			dRSetIdentity (RI);
			for (i=0; i<numc; i++) {
				dJointID c = dJointCreateContact (world,contactgroup,contact+i);
				dJointAttach (c,b1,b2);
				//if (show_contacts) dsDrawBox (contact[i].geom.pos,RI,ss);

				if (doFeedback && (b1==obj[selected].body || b2==obj[selected].body))
				{
					if (fbnum<MAX_FEEDBACKNUM)
					{
						feedbacks[fbnum].first = b1==obj[selected].body;
						dJointSetFeedback (c,&feedbacks[fbnum++].fb);
					}
					else fbnum++;
				}
				if(xsens1 || xsens2){
					// do we have a collisionSensor node? 
					// If so we need to send out the collisions its looking for
					int k;
					struct X3D_Contact *ct;
					dSurfaceParameters *surface;
					if(!static_contacts_initialized){
						//they are nodes, but static. We don't have a pure initialize function
						//in GeneratedCode (and too rushed to do one now), so will create one non-static,
						//and use it to initialize the statics
						static_contacts_initializer = createNewX3DNode0(NODE_Contact);
						static_contacts_initialized = TRUE;
					}
					static_contacts_n++;
					if(static_contacts_n >= 100) static_contacts_n = 99;
					k = static_contacts_n -1;
					ct = &static_contacts_p[k];
					memcpy(ct,static_contacts_initializer,sizeof(struct X3D_Contact));
					surface = &contact[i].surface;

					ct->appliedParameters.p = xcol ? xcol->appliedParameters.p : NULL;
					ct->appliedParameters.n = xcol ? xcol->appliedParameters.n : 0;
					ct->_appliedParameters = contact[i].surface.mode;
					ct->body1 = b1 ? dBodyGetData(b1) : NULL; //we set the user data to X3DRigidBody* when initializing the bodies
					ct->body2 = b2 ? dBodyGetData(b2) : NULL;
					ct->bounce = (float)surface->bounce;
					double2float(ct->contactNormal.c,contact[i].geom.normal,3);
					ct->depth =  (float)contact[i].geom.depth;
					ct->frictionCoefficients.c[0] = (float)surface->mu;
					ct->frictionCoefficients.c[1] = (float)surface->mu2;

					double2float(ct->frictionDirection.c,contact[i].fdir1,2);
					ct->geometry1 = X3D_NODE(xshape1);
					ct->geometry2 = X3D_NODE(xshape2);
					ct->minBounceSpeed = (float)surface->bounce_vel;
					double2float(ct->position.c,contact[i].geom.pos,3);
					ct->slipCoefficients.c[0] = (float)surface->mu;
					ct->slipCoefficients.c[1] = (float)surface->mu2;
					ct->softnessConstantForceMix = (float)surface->soft_cfm;
					ct->softnessErrorCorrection = (float)surface->soft_erp;
					ct->surfaceSpeed.c[0]= (float)surface->motion1;
					ct->surfaceSpeed.c[1]= (float)surface->motion2;
					if(xsens1 && xsens1->enabled){
						//BOMBING - CRoutes was bombing if I was resizing contacts.p and intersections.p in there
						//so I malloc once to 100=MAXCONTACTS
						if(xsens1->contacts.p == NULL)
							xsens1->contacts.p = malloc(100 * sizeof(void*));
						//if(xsens1->contacts.n == 0)
						//	xsens1->contacts.p = malloc((xsens1->contacts.n+1)*sizeof(void*));
						//else
						//	xsens1->contacts.p = realloc(xsens1->contacts.p,(xsens1->contacts.n+1)*sizeof(void*));
						xsens1->contacts.p[xsens1->contacts.n] = X3D_NODE(ct);
						xsens1->contacts.n++;
						//we mark these in do_CollisionSensor if contacts.n > 0
						//MARK_EVENT(X3D_NODE(xsens1),offsetof(struct X3D_CollisionSensor,contacts));
						//xsens1->isActive = TRUE;
						//MARK_EVENT(X3D_NODE(xsens1),offsetof(struct X3D_CollisionSensor,isActive));
						if(xsens1->intersections.p == NULL)
							xsens1->intersections.p = malloc(100 * sizeof(void*));
						//if(xsens1->intersections.n == 0)
						//	xsens1->intersections.p = malloc((xsens1->intersections.n+2)*sizeof(void*));
						//else
						//	xsens1->contacts.p = realloc(xsens1->intersections.p,(xsens1->intersections.n+1)*sizeof(void*));
						xsens1->intersections.p[xsens1->intersections.n] = X3D_NODE(xshape1);
						xsens1->intersections.n++;

					}
					if(xsens2 && xsens2->enabled && (xsens2 != xsens1 || (xsens1 && !xsens1->enabled))){
						if(xsens2->contacts.p == NULL)
							xsens2->contacts.p = malloc(100 * sizeof(void*));
						//if(xsens2->contacts.n == 0)
						//	xsens2->contacts.p = malloc((xsens2->contacts.n+1)*sizeof(void*));
						//else
						//	xsens2->contacts.p = realloc(xsens2->contacts.p,(xsens2->contacts.n+1)*sizeof(void*));
						xsens2->contacts.p[xsens2->contacts.n] = X3D_NODE(ct);
						xsens2->contacts.n++;
						//MARK_EVENT(X3D_NODE(xsens2),offsetof(struct X3D_CollisionSensor,contacts));
						//xsens2->isActive = TRUE;
						//MARK_EVENT(X3D_NODE(xsens2),offsetof(struct X3D_CollisionSensor,isActive));
					}
					if(xsens2 && xsens2->enabled){
						if(xsens2->intersections.p == NULL)
							xsens2->intersections.p = malloc(100 * sizeof(void*));
						//if(xsens2->intersections.n == 0)
						//	xsens2->intersections.p = malloc((xsens2->intersections.n+2)*sizeof(void*));
						//else
						//	xsens2->contacts.p = realloc(xsens2->intersections.p,(xsens2->intersections.n+1)*sizeof(void*));
						xsens2->intersections.p[xsens2->intersections.n] = X3D_NODE(xshape2);
						xsens2->intersections.n++;
					}
				}
			}
		}
	}
}

static int init_rbp_once = 0;
//static dThreadingImplementationID threading;
//static dThreadingThreadPoolID pool;
int init_rbp(){
	//we don't want to run physics if the scene has no physics stuff
	if(!init_rbp_once && world && space && contactgroup){
		init_rbp_once = TRUE;
		//c++: 
		dInitODE2(0);
		memset (obj,0,sizeof(obj));

		//if(0){
		//dThreadingImplementationID threading = dThreadingAllocateMultiThreadedImplementation();
		//dThreadingThreadPoolID pool = dThreadingAllocateThreadPool(4, 0, dAllocateFlagBasicData, NULL);
		//dThreadingThreadPoolServeMultiThreadedImplementation(pool, threading);
		//// dWorldSetStepIslandsProcessingMaxThreadCount(world, 1);
		//dWorldSetStepThreadingImplementation(world, dThreadingImplementationGetFunctions(threading), threading);
		//}
		//if(0) dAllocateODEDataForThread(dAllocateMaskAll);

	}
	return init_rbp_once;
}
void finish_rbp(){
	if(init_rbp_once){

		//dThreadingImplementationShutdownProcessing(threading);
		//dThreadingFreeThreadPool(pool);
		//dWorldSetStepThreadingImplementation(world, NULL, NULL);
		//dThreadingFreeImplementation(threading);


		dJointGroupDestroy (contactgroup);
		dSpaceDestroy (space);
		dWorldDestroy (world);
		dCloseODE();
		init_rbp_once = 0;
	}

}
enum {
FORCEOUT_ALL = 0xFFFFFFFF,
FORCEOUT_NONE = 0x00000000,
//MOTOR
FORCEOUT_motor1Angle	= 1 << 0,
FORCEOUT_motor1AngleRate= 1 << 1,
FORCEOUT_motor2Angle	= 1 << 2,
FORCEOUT_motor2AngleRate= 1 << 3,
FORCEOUT_motor3Angle	= 1 << 4,
FORCEOUT_motor3AngleRate= 1 << 5,
//DOUBLEAXISHING
FORCEOUT_body1AnchorPoint=1 << 6,
FORCEOUT_body1Axis		= 1 << 7,
FORCEOUT_body2AnchorPoint=1 << 8,
FORCEOUT_body2Axis		= 1 << 9,
FORCEOUT_hinge1Angle	= 1 << 10,
FORCEOUT_hinge1AngleRate= 1 << 11,
FORCEOUT_hinge2Angle	= 1 << 12,
FORCEOUT_hinge2AngleRate= 1 << 13,
//SINGLEAXIS
FORCEOUT_angle			= 1 << 14,
FORCEOUT_angleRate		= 1 << 15,
//SLIDER
FORCEOUT_separation		= 1 << 16,
FORCEOUT_separationRate	= 1 << 17,


};
static struct force_output_fieldname {
char *fieldname;
unsigned int bitmask;
} force_output_fieldnames [] = {
{"ALL",				FORCEOUT_ALL},
{"NONE",			FORCEOUT_NONE},
//MOTOR
{"motor1Angle",		FORCEOUT_motor1Angle},
{"motor1AngleRate",	FORCEOUT_motor1AngleRate},
{"motor2Angle",		FORCEOUT_motor2Angle},
{"motor2AngleRate",	FORCEOUT_motor2AngleRate},
{"motor3Angle",		FORCEOUT_motor3Angle},
{"motor3AngleRate",	FORCEOUT_motor3AngleRate},
//DOUBLEAXISHINGE
{"body1AnchorPoint"	,FORCEOUT_body1AnchorPoint},
{"body1Axis"		,FORCEOUT_body1Axis},
{"body2AnchorPoint"	,FORCEOUT_body2AnchorPoint},
{"body2Axis"		,FORCEOUT_body2Axis},
{"hinge1Angle"		,FORCEOUT_hinge1Angle},
{"hinge1AngleRate"	,FORCEOUT_hinge1AngleRate},
{"hinge2Angle"		,FORCEOUT_hinge2Angle},
{"hinge2AngleRate"	,FORCEOUT_hinge2AngleRate},
//SINGLE
{"angle"			,FORCEOUT_angle},
{"angleRate"		,FORCEOUT_angleRate},
//SLIDER
{"separation"		,FORCEOUT_separation},
{"separationRate"	,FORCEOUT_separationRate},



{NULL,0}
};
unsigned int forceout_from_names(int n, struct Uni_String **p){
	int i,j;
	unsigned int ret = 0;
	if(!strcmp(p[0]->strptr,"ALL")) return FORCEOUT_ALL;
	if(!strcmp(p[0]->strptr,"NONE")) return FORCEOUT_NONE;
	for(i=0;i<n;i++){
		//struct force_output_fieldname *fname;
		j=0;
		do{
			 if(!strcmp(p[i]->strptr,force_output_fieldnames[j].fieldname)){
				ret |= force_output_fieldnames[j].bitmask;
			 }
			 j++;
		}while(force_output_fieldnames[j].fieldname != NULL);
	}
	return ret;
}


void setTransformsAndGeom_E(dSpaceID space, void *csensor, struct X3D_Node* parent, struct X3D_Node **nodes, int n){
	// ATTEMPT 6
	// this is an initialzation step function, called once for program/scene run, not called again once _body is intialized
	// USING OCTAGA CONVENTION - only use initial CollidableOffset for offset
	// which initCollidable() copies to _initialTranslation, _initialRotation just once, 
	// - and zeros the regular translation, rotation for both CollidableOffset and CollidableShape
	// - either on compile_CollidableXXXX or in run_rigid_body() on initialization of _body
	// we can recurse if collidableOffset, although not branching recursion
	// - collidableShape is the leaf
	// http://ode.org/ode-latest-userguide.html#sec_10_7_7
	// geomTransform is used between RigidBody and shape geom so geom can have composite delta/shift/offset
	// see demo_boxstack 'x' keyboard option, which is for composite objects. 
	// phase I - get mass, collision geom, and visual to show up in the same place
	// phase II - harmonize with RigidBodyCollection->collidables [CollisionCollection] stack, which can 
	//    include static geometry not represented as RigidBodys. By harmonize I mean
	//    - detect if already generated collidable, and add RB mass

	int kn;
	for(kn=0;kn<n;kn++){
		dGeomID gid = NULL; //top level geom
		struct X3D_Node *node = nodes[kn];
		if(node)
		if(node->_nodeType == NODE_CollidableShape || node->_nodeType == NODE_CollidableOffset || node->_nodeType == NODE_CollisionSpace){
			float *translation, *rotation;
			struct X3D_CollidableOffset *collidable = (struct X3D_CollidableOffset *)node;
			translation = collidable->translation.c;
			rotation = collidable->rotation.c;

			switch(node->_nodeType){
				case NODE_CollidableShape:
					{
					struct X3D_CollidableShape *cshape = (struct X3D_CollidableShape *)node;
					gid = cshape->_geom;
					//printf("handle collidable shape\n");
					if(!cshape->_geom){
						struct X3D_Shape *shape = (struct X3D_Shape*)cshape->shape;
						dGeomID shapegid;
						gid = dCreateGeomTransform (space); //dSpaceID space);
						dGeomTransformSetCleanup (gid,1);

						if(shape && shape->geometry){

							dReal sides[3];
							switch(shape->geometry->_nodeType){
								case NODE_Box:
									{
										struct X3D_Box *box = (struct X3D_Box*)shape->geometry;
										sides[0] = box->size.c[0]; sides[1] = box->size.c[1], sides[2] = box->size.c[2];
										shapegid = dCreateBox(0,sides[0],sides[1],sides[2]);
										//printf("shape box\n");
									}
									break;
								case NODE_Cylinder:
									{
										struct X3D_Cylinder *cyl = (struct X3D_Cylinder*)shape->geometry;
										sides[0] = cyl->radius;
										sides[1] = cyl->height;
										shapegid = dCreateCylinder(0,sides[0],sides[1]);
									}
									break;
								//case convex - not done yet, basically indexedfaceset or triangleSet?
								case NODE_TriangleSet:
									{
										//see ODE demo_heightfield.cpp 
										int j,k;
										dTriMeshDataID new_tmdata;
										struct X3D_TriangleSet *tris = (struct X3D_TriangleSet*)shape->geometry;
										struct X3D_Coordinate *coord = (struct X3D_Coordinate *)tris->coord;
										int index_count = coord->point.n;
										dTriIndex * indices = malloc(index_count*sizeof(dTriIndex));
										for(j=0;j<index_count/3;j++){
											for( k=0;k<3;k++)
												indices[j*3 +k] = j*3 + k;
										}
										new_tmdata = dGeomTriMeshDataCreate();
										dGeomTriMeshDataBuildSingle(new_tmdata, coord->point.p, 3 * sizeof(float), coord->point.n, 
											&indices[0], index_count, 3 * sizeof(dTriIndex));

										shapegid = dCreateTriMesh(0, new_tmdata, 0, 0, 0);
										//free(indices); will need to clean up at program end, ODE assumes this and the point.p hang around
										//printf("shape trimesh\n");
									}
									break;
								case NODE_Sphere:
								default:
									{
										struct X3D_Sphere *sphere = (struct X3D_Sphere*)shape->geometry;
										sides[0] = sphere->radius;
										shapegid = dCreateSphere(0,sides[0]);
										//printf("shape sphere\n");
									}
									break;
							}
						}
						cshape->_geom = gid; //we will put trans wrapper whether or not there's an offset parent.
						cshape->_csensor = csensor;
						dGeomTransformSetGeom (gid,shapegid);
						dGeomSetData(gid,cshape); //so on collision nearCallback we know which collisionSensor->contacts to set
					}
					}
					break;
				case NODE_CollisionSpace:
					{
						struct X3D_CollisionSpace *cspace = (struct X3D_CollisionSpace *)node;

						//recurse to leaf-node collidableShape
						if(!cspace->_space){
							cspace->_space = dHashSpaceCreate (space);
							dGeomSetData(cspace->_space,cspace);
						}
						//printf("handle collisionspace\n");
						setTransformsAndGeom_E(cspace->_space,csensor,X3D_NODE(cspace),cspace->collidables.p,1);

					}
					break;
				case NODE_CollidableOffset:
					{
						struct X3D_CollidableOffset *coff = (struct X3D_CollidableOffset *)node;
						//printf("handle collidableoffset\n");
						//recurse to leaf-node collidableShape
						struct X3D_Node *nodelist[1];
						struct X3D_Node *nodelistitem = X3D_NODE(coff->collidable);
						nodelist[0] = nodelistitem;
						setTransformsAndGeom_E(space,csensor,X3D_NODE(coff),nodelist,1);
						gid = coff->_geom;
					}
					break;
				default:
					break;
			}
			if(gid){
				switch(parent->_nodeType){
					case NODE_CollidableOffset:
					{
						//snippet from ODE demo_boxstack.cpp cmd == 'x' {} section
						dMatrix3 Rtx;
						struct X3D_CollidableOffset *coff = (struct X3D_CollidableOffset *)parent;
						float *translation, *rotation;
						translation = coff->_initialTranslation.c;
						rotation = coff->_initialRotation.c;
						dGeomSetPosition (gid,translation[0],translation[1],translation[2]);
						dRFromAxisAndAngle (Rtx,rotation[0],rotation[1],rotation[2],rotation[3]);
						dGeomSetRotation (gid,Rtx);
						coff->_geom = gid;
						//printf("parent collidableoffset\n");
					}
					break;
					case NODE_RigidBody:
					{
						struct X3D_RigidBody *rb = (struct X3D_RigidBody *)parent;
						dGeomSetBody (gid,rb->_body);
						//printf("parent rigidbody\n");
					}
					//fallthrough to transform initializing below
					case NODE_CollisionSpace:
						//printf("parent space\n");
					case NODE_CollisionCollection:
						//printf("parent collisionCollection\n");
					case NODE_RigidBodyCollection:
					{
						//printf("parent rigidbodycollection\n");
						if(node->_nodeType == NODE_CollidableShape){
							//we have a collidable, but we aren't inside a rigidbody
							//so we want to keep and use any translate/rotate for global placement
							struct X3D_CollidableShape *cshape2 = (struct X3D_CollidableShape *)node;
							if(!cshape2->_initialized){

								float *translation, *rotation, *initialtranslation, *initialrotation;
								translation = cshape2->translation.c;
								rotation = cshape2->rotation.c;
								initialtranslation = cshape2->_initialTranslation.c;
								initialrotation = cshape2->_initialRotation.c;
								if(!vecsame3f(initialtranslation,translation)){
									dGeomSetPosition (gid,translation[0],translation[1],translation[2]);
									veccopy3f(initialtranslation,translation);
								}
								if(!vecsame4f(initialrotation,rotation)){
									dMatrix3 Rtx;
									dRFromAxisAndAngle (Rtx,rotation[0],rotation[1],rotation[2],rotation[3]);
									dGeomSetRotation (gid,Rtx);
									veccopy4f(initialrotation,rotation);
								}
								cshape2->_initialized = TRUE;
								//printf("initializingB\n");
							}
						}

					}
					default:
					break;
				}
			}
		}
	}
}

static int pause = FALSE;
static double STEP_SIZE = .02; //seconds
/* http://www.ode.org/ode-0.039-userguide.html#ref27
3.10. Typical simulation code
A typical simulation will proceed like this:
1.Create a dynamics world.
2.Create bodies in the dynamics world.
3.Set the state (position etc) of all bodies.
4.Create joints in the dynamics world.
5.Attach the joints to the bodies.
6.Set the parameters of all joints.
7.Create a collision world and collision geometry objects, as necessary.
8.Create a joint group to hold the contact joints.
9.Loop:
1.	Apply forces to the bodies as necessary.
2.	Adjust the joint parameters as necessary.
3.	Call collision detection.
4.	Create a contact joint for every collision point, and put it in the contact joint group.
5.	Take a simulation step.
6.	Remove all joints in the contact joint group.
10.Destroy the dynamics and collision worlds.

UPDATING NODES when NODE_NEEDS_COMPILING:
Usually when you want to change a parameter on a running physics simulation, you 
don't want the simulation to completely start over. You want it to carry on. The
physics engine holds state for where things are. 

In freewrl > propagate_events_B() > update_node(toNode); we flag the whole target
node as needing a recompile, even when only one field was changed.
To tell later, when compile_ recompiling the target node, which fields have changed
we change the PERL node struct to have __oldfieldvalue and each time we recompile
we save the current value of the field. So next time we compile_ the node,
we can see if newfield == __oldfieldvalue, and if not, that field changed - apply the change.
When we don't check, its because no harm is done by re-setting the field even if not changed.

For RBPhysics nodes, there are lots of fields that can be harmlessly reset in the
middle of a simulation.
But there are some fields that should only be reset if the field changed 
ORIC - Only Reset If Changed
There's a pattern to them: 
1. they relate to setting up the initial geometry poses, which are done
    in global coordinates, not geometry or shape or 'local' coordinates, so once the simm starts
	a change to these makes a mess
2. no need to recompile Joint when child Bodies are recompiled - unless the body node (node* pointer) is different 
   - because we update the physics state sufficiently when compiling the body

I'll list (my guess at) the ORIC (Only Reset If Changed) fields here:

BallJoint
anchorPoint
body1,body2

CollidableShape,CollidableOffset
(recompile like Transform node)
x but don't flag parent X3DBody as needing to recompile

DoubleAxisHingeJoint
anchorPoint
axis1,2
body1,body2

MotorJoint
motor1Axis,motor2Axis,motor3Axis
axisAngle1,axisAngle2,axisAngle3
body1,body2

RigidBody
centerOfMass
finiteRotationAxis
linearVelocity
angularVelocity
orientation
position

SingleAxisHinge
anchorPoint
axis
body1,body2

SliderJoint
axis
body1,body2

UniversalJoint
anchorPoint
axis1,2,3
body1,body2

Total: 31 ORIC fields
(plus lots of other fields that are wastefully reset on a generic recompile)

There are various options/possibilities/strategies for recording and recovering which fields were changed:
1. __oldvalue fields for ORIC fields. Works well for scalars, but not MF/.n,.p fields (like forceOutput)
2. create a bitflag field, one bit for each field in the node 
	(struct X3D_EspduTransform has 90 fields, and several are in the 30-40 range, would need 3 int32 =12 bytes)
3. refactor freewrl code to include a bitflag in each field (like Script and Proto field structs)
4. as in 2,3 except node would declare a __bitflag field only if it cares, and update_node would
	check for that field and set the field bit only if it has the __bitflag field.
	a) in the default part of the node struct, a pointer field called __bitflag which is normally null
		- and malloced if needed
	b) in the node-specific part, an MFInt32 field, and n x 32 bits are available
Issue with 2,3,4: either you need all the bitflags set on startup (to trigger all fields compile)
	or else you need a way to signal when its a partial recompile vs full recompile
	- perhaps a special _change value on startup.

DECISION: we'll do the #1 __oldvalue technique.

*/
int static_did_physics_since_last_Tick = FALSE;
void rbp_run_physics(){
	/*	called once per frame.
		assumes we have lists of registered worlds (x3drigidbodycollections) and 
			registered collisionSensors if any
		for each world, 
			- we drill down and check each thing and if not initialized, we initialize it 
			- we run the collision and simulation steps
			- we output to any collisionSensors
	*/
	int nstep;
	ppComponent_RigidBodyPhysics p;
	p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	
	nstep = 1;
	if(1){
		//situation: physics simulations need constant size time steps ie STEP_SIZE seconds .02
		//goal: make the simulation speed match wall clock
		//method: from render thread, skip or add simulation steps as/when needed,
		// so average wallclock time between steps ~= stepsize (seconds)
		//advantage: no thread marshalling needed
		//versus: separate thread which could sleep, sim, sleep, sim .. to match STEP_SIZE 
		//   - disadvantage of separate thread: would need to block threads while moving data between the threads
		double thistime;
		static double lasttime;
		static double remainder = 0.0;
		static int done_once = 0;

		thistime = TickTime();
		if(done_once > 0){
			double deltime = thistime - lasttime + remainder;
			nstep = (int) floor( deltime / STEP_SIZE );
			remainder = deltime - (nstep * STEP_SIZE);
		}
		lasttime = thistime;
		done_once = 1;
	}
	//printf("%d ",nstep);
	if(nstep < 1) return;
	//see nstep below when calling dworldquickstep we loop over nstep
	static_did_physics_since_last_Tick = TRUE;


	if(init_rbp()){
		int i,j,k;
		struct X3D_RigidBodyCollection *x3dworld;

		if(x3dcollisionsensors){
			struct X3D_CollisionSensor *csens;
			for(i=0;i<x3dcollisionsensors->n;i++){
				csens = vector_get(struct X3D_CollisionSensor*,x3dcollisionsensors,i);
				//clear contacts from last frame 
				csens->contacts.n = 0;
				// leave at 100 FREE_IF_NZ(csens->contacts.p);
				//clear intersections from last frame
				csens->intersections.n = 0;
				if(csens->collider){
					//this doesn't run the collision sensor, just makes sure it's 'compiled' / initialized
					//so no need to check for ->enabled
					struct X3D_CollisionCollection *ccol = (struct X3D_CollisionCollection *)csens->collider;
					ccol->_csensor = csens;
				}
			}
		}

		if(x3dcollisioncollections){
			// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/rigid_physics.html#CollisionCollection
			struct X3D_CollisionCollection *ccol;
			struct X3D_CollisionSensor *csens;
			void * csensor;
			for(i=0;i<x3dcollisioncollections->n;i++){
				ccol = vector_get(struct X3D_CollisionCollection*,x3dcollisioncollections,i);

				csens = ccol->_csensor;
				if(NNC(ccol)){
					unsigned int mask = 0;
					int k;
					for(k=0;k<ccol->appliedParameters.n;k++){
						//I shamefully copied and pasted enums from ode header without trying to understand them
						// http://ode.org/ode-latest-userguide.html#sec_7_3_7
						const char *ap = ccol->appliedParameters.p[k]->strptr;
						if(!strcmp(ap,"BOUNCE")){
							mask |= dContactBounce;
						}else if(!strcmp(ap,"USER_FRICTION")){
							mask |= dContactFDir1;
						}else if(!strcmp(ap,"FRICTION_COEFFICIENT-2")){
							mask |= dContactMu2;
						}else if(!strcmp(ap,"ERROR_REDUCTION")){
							mask |= dContactSoftERP;
						}else if(!strcmp(ap,"CONSTANT_FORCE")){
							mask |= dContactSoftCFM;
						}else if(!strcmp(ap,"SPEED-1")){
							mask |= dContactMotion1;
						}else if(!strcmp(ap,"SPEED-2")){
							mask |= dContactMotion2;
						}else if(!strcmp(ap,"SLIP-1")){
							mask |= dContactSlip1;
						}else if(!strcmp(ap,"SLIP-2")){
							mask |= dContactSlip2;
						}
					}
					ccol->_appliedParametersMask = mask;
					// Q. how to 'enable=FALSE' a collisionCollection?
					// H: ODE category bits? dug9 Dec 2016 I didn't implement.
					MNC(ccol);
				}
				csensor = csens ? (void*)csens : (void*)ccol;  //user data assigned to geom, for recovery from collided geoms in nearCallback
				setTransformsAndGeom_E(space, csensor, X3D_NODE(ccol), ccol->collidables.p, ccol->collidables.n);
			}
		}


		for(j=0;j<vectorSize(x3dworlds);j++){
			struct X3D_RigidBody *x3dbody;
			struct X3D_CollidableOffset *x3doffset;
			struct X3D_CollidableShape *x3dcshape;
			x3dworld = (struct X3D_RigidBodyCollection*)vector_get(struct X3D_Node*,x3dworlds,j);
			//Collidable -> rigidbody 
			if(!x3dworld->enabled) continue;
			//if(!x3dworld->_world){
			if(NNC(x3dworld) || x3dworld->_world == NULL){
				x3dworld->_world = world;
				if(x3dworld->contactSurfaceThickness > 0)
					dWorldSetContactSurfaceLayer (x3dworld->_world, x3dworld->contactSurfaceThickness);
				dWorldSetGravity (x3dworld->_world, x3dworld->gravity.c[0], x3dworld->gravity.c[1], x3dworld->gravity.c[2]);
				dWorldSetERP (x3dworld->_world, x3dworld->errorCorrection);
				dWorldSetCFM (x3dworld->_world, x3dworld->constantForceMix);
				dWorldSetAutoDisableFlag (x3dworld->_world, x3dworld->autoDisable);
				dWorldSetAutoDisableLinearThreshold (x3dworld->_world, x3dworld->disableLinearSpeed);
				dWorldSetAutoDisableAngularThreshold (x3dworld->_world, x3dworld->disableAngularSpeed);
				dWorldSetAutoDisableTime (x3dworld->_world, x3dworld->disableTime);
				if(x3dworld->maxCorrectionSpeed > -1)
					dWorldSetContactMaxCorrectingVel (x3dworld->_world, x3dworld->maxCorrectionSpeed);
				MNC(x3dworld);
			}

			if(x3dworld->set_contacts.n){
				//someone in javascript / sai sent us some extra contacts to add as contact joints
				int kk;
				for (kk=0; kk < x3dworld->set_contacts.n; kk++) {
					struct X3D_RigidBody *body1, *body2;
					struct X3D_Contact *ct = (struct X3D_Contact *)x3dworld->set_contacts.p[kk];
					dContact contact;
					dBodyID b1, b2;
					dJointID c = dJointCreateContact (world,contactgroup,&contact);

					contact.surface.mode = ct->_appliedParameters; //dContactBounce; // | dContactSoftCFM;
					contact.surface.mu = ct->slipCoefficients.c[0]; //dInfinity;
					contact.surface.mu2 = ct->slipCoefficients.c[1];
					contact.surface.bounce = ct->bounce;
					contact.surface.bounce_vel = ct->minBounceSpeed;
					contact.surface.soft_cfm = ct->softnessConstantForceMix;
					contact.surface.soft_erp = ct->softnessErrorCorrection;
					contact.surface.motion1 = ct->surfaceSpeed.c[0];
					contact.surface.motion2 = ct->surfaceSpeed.c[1];
					float2double(contact.geom.pos,ct->position.c,3);
					float2double(contact.fdir1,ct->frictionDirection.c,2);
					float2double(contact.geom.normal,ct->contactNormal.c,3);

					body1 = (struct X3D_RigidBody*)ct->body1;
					body2 = (struct X3D_RigidBody*)ct->body2;
					b1 = body1 ? body1->_body : NULL;
					b2 = body2 ? body2->_body : NULL;
					if (b1 && b2 && dAreConnectedExcluding (b1,b2,dJointTypeContact)) continue;
					dJointAttach (c,b1,b2);
				}
				x3dworld->set_contacts.n = 0;
			}
			//update bodies
			for(i=0;i<x3dworld->bodies.n;i++){
				int isNewBody = FALSE;
				int method_anchor_fixed_with_fixed_joint = 1;
				x3dbody = (struct X3D_RigidBody*)x3dworld->bodies.p[i];
				if(method_anchor_fixed_with_fixed_joint){
					if(!x3dbody->_body){
						x3dbody->_body = dBodyCreate (x3dworld->_world);
						dBodySetData (x3dbody->_body,(void*) x3dbody);
						if(x3dbody->fixed){
							//a few joints - maybe just hinge2? - need 2 bodies or ODE ASSERTS.
							//anchor with unrecommended fixed joint
							//note we still need MASS on the fixed body, otherwise it jitters
							dJointID anchorID = dJointCreateFixed(x3dworld->_world,x3dworld->_group);
							dJointAttach(anchorID,x3dbody->_body,0);
							dJointSetFixed (anchorID);
						}
						isNewBody = TRUE;
					}
				}else{
					if(!x3dbody->_body && !x3dbody->fixed){
						//_body == 0 tells ODE its fixed geometry
						x3dbody->_body = dBodyCreate (x3dworld->_world);
						dBodySetData (x3dbody->_body,(void*) x3dbody);
						isNewBody = TRUE;
					}
				}

				if(x3dbody->enabled)
					dBodyEnable (x3dbody->_body);
				else
					dBodyDisable(x3dbody->_body);
				x3dcshape = NULL;
				x3doffset = NULL;
				if(isNewBody){
					// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/rigid_physics.html#CollidableOffset
					// collidableOffset can recurse ie it's collidable can be either another collidableOffset, or a collidableShape
					// however it's not a branching recurse.
					// ODE http://ode.org/ode-latest-userguide.html#sec_10_7_7
					// ode has a geomTransform


					// ATTEMPT 6, Dec 8 and 9, 10 2016
					//ATTEMPT 6 WILL USE OCTAGA CONVENTION for CollidableOffset
					//- ignore CollidableShape pose on init
					//- use CollidableOffset pose for compositing offset/delta
					//- write to only the top Collidable transform fields
					//- apply the offset -if CollidableOffset- before writing the transform fields
					//- on render, use only the top level transform (which will include any offset if present)
					//- a way to ensure, is when compile_ collideable, save to __initials only if Offset,
					//    then zero transform without marking event
					// based on ODE test demo_boxstack.cpp 'x' option for composite object
					// - it seems to be doing within-RB transforms during composition
					// - then transforming the rigidbody during physics
					// - 2 ways: a) USE_GEOM_OFFSET b) geomTransform
					// - and in both cases the composition transforms are applied to the collidable shape geom
					// - geomTransform is just there to force separation between RB and collidable 
					//		so they don't share transform
					// IDEA: 
					// 1. stop ODE from transmitting initial x3dbody pose to geom and vice versa
					//   for the purposes of allowing composite objects, and do that 
					//   by always inserting exactly  one geomTransform wrapper per RigidBody
					//   with no transform applied to the geomTransform
					//   see ODE demo_boxstack.cpp 'x' option
					// 2. when recursing down Collidables stack for first traverse/initialization, 
					//    a) on init of Collidable, save CollidableOffset to initialTranslation,_initialRotation,
					//			zero the translation,rotation fields of CollidableOffset and CollidableShape
					//    b) apply only CollidableOffset initial transform (saved in _initialTranslation,_initialRotation)
					//         to shape, to position it relative to its parent/grandParent RigidBody,
					//         and apply it only to leaf geom ie box, sphere, ...
					// 3. on MARK_EVENT set RigidBody pose and MARK, 
					//    and take RigidBody pose, apply _initialTranslation, _initialRotation of top collidable,
					//    and set on top collidable* translation,rotation and MARK_EVENT
					// 4. on scenegraph traversal
					//	  transform using collidable transforms like a normal transform stack
					// This should allow composite geom RigidBodys
					// visualization - as usual either expose collidable in scenegraph, or route from them
					//   to individual parts, or from the corresponding rigidbody to a wrapper transform on other geom
					//   that doesn't need the individual part transforms
					// Phase I - get SingleHingeJoint type scenes to work
					// Phase II - harmonize with RigidBodyCollection->collidables [CollidableCollection] scenes

					float *translation, *rotation;
					translation = x3dbody->position.c;
					rotation = x3dbody->orientation.c;

					for(k=0;k<x3dbody->geometry.n;k++){
						//iterate over composite geometry
						//Phase I: just do _geom if not already done, and do both collidable geom and mass
						//   composition in one callstack
						//Phase II option: recurse even if _geom set by RBP->collidables separate traverse/callstack
						//   so we can set mass to the same pose
						struct X3D_Node *nodelistitem;
						struct X3D_Node *nodelist[1];
						struct X3D_CollidableOffset* collidable = (struct X3D_CollidableOffset*)x3dbody->geometry.p[k];
						nodelistitem = X3D_NODE(collidable);
						nodelist[0] = nodelistitem;
						setTransformsAndGeom_E(space,NULL,X3D_NODE(x3dbody), nodelist,1);
					}
					if(verify_translate(translation)){
						//printf("verified translation= %f %f %f\n",translation[0],translation[1],translation[2]);
						dBodySetPosition (x3dbody->_body, (dReal)translation[0],(dReal)translation[1],(dReal)translation[2]);
					}
					if(verify_rotate(rotation)){
						//printf("verified reotation= %f %f %f\n",rotation[0],rotation[1],rotation[2]);
						if(1){
							dReal dquat[4];
							Quaternion quat;
							vrmlrot_to_quaternion(&quat,rotation[0],rotation[1],rotation[2],rotation[3]);
							dquat[0] = quat.w; dquat[1] = quat.x, dquat[2] = quat.y, dquat[3] = quat.z;
							dBodySetQuaternion(x3dbody->_body,dquat);
						}else{
							dMatrix3 R;
							dRFromAxisAndAngle (R,rotation[0],rotation[1],rotation[2],rotation[3]);
							dBodySetRotation(x3dbody->_body,R);
						}
					}

				}
				if(x3dbody->_body){
					//not fixed
					if(NNC(x3dbody)){
						float speed;
						//not fixed
						if(x3dbody->autoDamp){
							dBodySetAngularDamping(x3dbody->_body, x3dbody->angularDampingFactor);
							dBodySetLinearDamping(x3dbody->_body, x3dbody->linearDampingFactor);
						}else{
							dBodySetDampingDefaults(x3dbody->_body);
						}
						if(x3dbody->massDensityModel){
							dReal sides[3];
							dMass m;
							switch(x3dbody->massDensityModel->_nodeType){
								case NODE_Box:
									{
										struct X3D_Box *box = (struct X3D_Box*)x3dbody->massDensityModel;
										sides[0] = box->size.c[0]; sides[1] = box->size.c[1], sides[2] = box->size.c[2];
										dMassSetBox (&m,DENSITY,sides[0],sides[1],sides[2]);
									}
									break;
								case NODE_Cylinder:
									{
										struct X3D_Cylinder *cyl = (struct X3D_Cylinder*)x3dbody->massDensityModel;
										sides[0] = cyl->radius;
										sides[1] = cyl->height;
										dMassSetCylinder (&m,DENSITY,3,sides[0],sides[1]);
									}
									break;
								//case convex - not done yet, basically indexedfaceset
								case NODE_Sphere:
									{
										struct X3D_Sphere *sphere = (struct X3D_Sphere*)x3dbody->massDensityModel;
										sides[0] = sphere->radius;
										dMassSetSphere (&m,DENSITY,sides[0]);
									}
									break;
								default:
									{
										// http://ode.org/ode-latest-userguide.html#sec_9_2_0
										// 9.2 Mass Functions
										float *I = x3dbody->inertia.c;
										dMassSetParameters (&m, DENSITY,
											0.0, 0.0, 0.0,  //center of gravity - we'll adjust below in dMassTranslate
											I[0], I[1], I[2], //diagonal elements of 3x3 inertial tensor
											I[3], I[4], I[5]); //upper diagonal of 3x3 inertial tensor
									}
									break;
							}
							dMassAdjust(&m,x3dbody->mass);
							dMassTranslate (&m,x3dbody->centerOfMass.c[0],x3dbody->centerOfMass.c[1],x3dbody->centerOfMass.c[2]);
							dBodySetMass (x3dbody->_body, &m);
						}else{
							dMass m;
							dMassSetSphere (&m,DENSITY,.01);
							dMassAdjust(&m,x3dbody->mass);
							dMassTranslate (&m,x3dbody->centerOfMass.c[0],x3dbody->centerOfMass.c[1],x3dbody->centerOfMass.c[2]);
							dBodySetMass (x3dbody->_body, &m);
						}
						if(x3dbody->useFiniteRotation){
							if(!vecsame3f(x3dbody->__old_finiteRotationAxis.c,x3dbody->finiteRotationAxis.c)){
								dBodySetFiniteRotationAxis (x3dbody->_body, x3dbody->finiteRotationAxis.c[0],x3dbody->finiteRotationAxis.c[1],x3dbody->finiteRotationAxis.c[2]);
								veccopy3f(x3dbody->__old_finiteRotationAxis.c,x3dbody->finiteRotationAxis.c);
							}
						}
						dBodySetFiniteRotationMode (x3dbody->_body, x3dbody->useFiniteRotation ? 1 : 0);

						//gravity?
						if(!x3dbody->useGlobalGravity){
							dBodySetGravityMode(x3dbody->_body,0);
						}
						//position and orientation set once in if(!_geom) above
						//add any per-step per-body forces
						speed = veclength3f(x3dbody->linearVelocity.c);
						if(speed > .001f){
							if(!vecsame3f(x3dbody->__old_linearVelocity.c,x3dbody->linearVelocity.c)){
								dBodySetLinearVel(x3dbody->_body, x3dbody->linearVelocity.c[0],x3dbody->linearVelocity.c[1],x3dbody->linearVelocity.c[2]);
								veccopy3f(x3dbody->__old_linearVelocity.c,x3dbody->linearVelocity.c);
							}
						}
						speed = veclength3f(x3dbody->angularVelocity.c);
						if(speed > .0001f){
							if(!vecsame3f(x3dbody->__old_angularVelocity.c,x3dbody->angularVelocity.c)){
								dBodySetAngularVel(x3dbody->_body, x3dbody->angularVelocity.c[0],x3dbody->angularVelocity.c[1],x3dbody->angularVelocity.c[2]);
								veccopy3f(x3dbody->__old_angularVelocity.c,x3dbody->angularVelocity.c);
							}
						}
						dBodySetAutoDisableFlag (x3dbody->_body, x3dbody->autoDisable ? 1 : 0);
						dBodySetAutoDisableLinearThreshold (x3dbody->_body, x3dbody->disableLinearSpeed);
						dBodySetAutoDisableAngularThreshold (x3dbody->_body, x3dbody->disableAngularSpeed);
						dBodySetAutoDisableTime (x3dbody->_body, x3dbody->disableTime);
						if(x3dbody->forces.n){
							//the engine's force accumulator is zeroed after each step, so for these
							//constant forces you have to re-add them on each step
							int kf;
							for(kf=0;kf<x3dbody->forces.n;kf++){
								float *force = x3dbody->forces.p[kf].c;
								dBodyAddForce(x3dbody->_body, force[0], force[1], force[2]);
							}
						}
						if(x3dbody->torques.n){
							//the engine's torque accumulator is zeroed after each step, so for these
							//constant torques you have to re-add them on each step
							int kf;
							for(kf=0;kf<x3dbody->torques.n;kf++){
								float *torque = x3dbody->torques.p[kf].c;
								dBodyAddTorque(x3dbody->_body, torque[0], torque[1], torque[2]);
							}
						}
					} //if NCC(x3dbody)

				} // if x3dbody->_body (not fixed)

			} //bodies
			//update joints
			for(i=0;i<x3dworld->joints.n;i++){
				struct X3D_Node *joint = (struct X3D_Node*)x3dworld->joints.p[i];
				// 	render_node(joint); //could use virt function? I'll start without virt
				switch(joint->_nodeType){
					case NODE_BallJoint:
						//render_BallJoint(joint); 
						{
							dJointID jointID;
							struct X3D_BallJoint *jnt = (struct X3D_BallJoint*)joint;
							if(!jnt->_joint){
								dBodyID body1ID, body2ID;
								struct X3D_RigidBody *xbody1, *xbody2;
								xbody1 = (struct X3D_RigidBody*)jnt->body1;
								xbody2 = (struct X3D_RigidBody*)jnt->body2;
								//allow for MULL body on one side of joint, to fix to static environment
								body1ID = xbody1 ? xbody1->_body : NULL;
								body2ID = xbody2 ? xbody2->_body : NULL;
								jointID = dJointCreateBall (x3dworld->_world,x3dworld->_group);
								dJointAttach (jointID,body1ID,body2ID);
								jnt->_joint = jointID;
								dJointSetBallAnchor(jnt->_joint, jnt->anchorPoint.c[0],jnt->anchorPoint.c[1], jnt->anchorPoint.c[2]);
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);
							}
							if(NNC(jnt)){
								if(!vecsame3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c)){
									dJointSetBallAnchor(jnt->_joint, jnt->anchorPoint.c[0],jnt->anchorPoint.c[1], jnt->anchorPoint.c[2]);
									veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								}
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);
								MNC(jnt);
							}
						}
						break;
					case NODE_SingleAxisHingeJoint:
						{
							dJointID jointID;
							struct X3D_SingleAxisHingeJoint *jnt = (struct X3D_SingleAxisHingeJoint*)joint;
							if(!jnt->_joint){
								dBodyID body1ID, body2ID;
								struct X3D_RigidBody *xbody1, *xbody2;
								xbody1 = (struct X3D_RigidBody*)jnt->body1;
								xbody2 = (struct X3D_RigidBody*)jnt->body2;
								//allow for MULL body on one side of joint, to fix to static environment
								body1ID = xbody1 ? xbody1->_body : NULL;
								body2ID = xbody2 ? xbody2->_body : NULL;
								jointID = dJointCreateHinge (x3dworld->_world,x3dworld->_group);
								dJointAttach (jointID,body1ID,body2ID);
								jnt->_joint = jointID;
								//veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								//veccopy3f(jnt->__old_axis.c,jnt->axis.c);
							}
							if(NNC(jnt)){
								float axislen;
								if(!vecsame3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c)){
									dJointSetHingeAnchor (jointID,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								}
								axislen = veclength3f(jnt->axis.c);
								if(axislen < .1){
									//specs say 0 0 0 is default but that's garbage, should be 0 0 1
									jnt->axis.c[0] = jnt->axis.c[1] = 0.0f;
									jnt->axis.c[2] = 1.0f;
								}
								if(!vecsame3f(jnt->__old_axis.c,jnt->axis.c)){
									dJointSetHingeAxis (jointID,jnt->axis.c[0],jnt->axis.c[1],jnt->axis.c[2]);
									veccopy3f(jnt->__old_axis.c,jnt->axis.c);
								}
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);
								dJointSetHingeParam (jnt->_joint,dParamLoStop,jnt->minAngle);
								dJointSetHingeParam (jnt->_joint,dParamHiStop,jnt->maxAngle);
								dJointSetHingeParam (jnt->_joint,dParamBounce,jnt->stopBounce);
								dJointSetHingeParam (jnt->_joint,dParamStopERP,jnt->stopErrorCorrection);
								MNC(jnt);
							}
						}
						break;
					case NODE_DoubleAxisHingeJoint:
						{
							//see the ODE demo_buggy for Hinge2 example of steerable wheel
							dJointID jointID;
							double avel;
							int method_extension_axis1Angle;
							struct X3D_DoubleAxisHingeJoint *jnt = (struct X3D_DoubleAxisHingeJoint*)joint;

							if(!jnt->_joint){
								dBodyID body1ID, body2ID;
								struct X3D_RigidBody *xbody1, *xbody2;
								xbody1 = (struct X3D_RigidBody*)jnt->body1;
								xbody2 = (struct X3D_RigidBody*)jnt->body2;
								//allow for MULL body on one side of joint, to fix to static environment
								body1ID = xbody1 ? xbody1->_body : NULL;
								body2ID = xbody2 ? xbody2->_body : NULL;
								jointID = dJointCreateHinge2 (x3dworld->_world,x3dworld->_group);
								//body1 should be the car, body2 the wheel
								dJointAttach (jointID,body1ID,body2ID);
								jnt->_joint = jointID;
								dJointSetHinge2Anchor(jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
								
								//axis 1 should be the (vertical) steering axis
								dJointSetHinge2Axis1(jnt->_joint,jnt->axis1.c[0],jnt->axis1.c[1],jnt->axis1.c[2]);
								//axis 2 should be the (horizontal) wheel axle
								dJointSetHinge2Axis2(jnt->_joint,jnt->axis2.c[0],jnt->axis2.c[1],jnt->axis2.c[2]);

							}
							if(NNC(jnt)){
								float axislen = veclength3f(jnt->axis1.c);
								if(axislen < .1){
									//specs say 0 0 0 is default but that's garbage, should be 0 0 1
									jnt->axis1.c[0] = jnt->axis1.c[1] = 0.0f;
									jnt->axis1.c[2] = 1.0f;
								}
								if(!vecsame3f(jnt->__old_axis1.c,jnt->axis1.c)){
									dJointSetHinge2Axis1(jnt->_joint,jnt->axis1.c[0],jnt->axis1.c[1],jnt->axis1.c[2]);
									veccopy3f(jnt->__old_axis1.c,jnt->axis1.c);
								}
								if(!vecsame3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c)){
									dJointSetHinge2Anchor(jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								}
								if(!vecsame3f(jnt->__old_axis2.c,jnt->axis2.c)){
									dJointSetHinge2Axis2(jnt->_joint,jnt->axis2.c[0],jnt->axis2.c[1],jnt->axis2.c[2]);
									veccopy3f(jnt->__old_axis2.c,jnt->axis2.c);
								}
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);
								dJointSetHinge2Param (jnt->_joint,dParamBounce1,jnt->stopBounce1);
								dJointSetHinge2Param (jnt->_joint,dParamStopERP1,jnt->stopErrorCorrection1);
								dJointSetHinge2Param (jnt->_joint,dParamSuspensionERP,jnt->suspensionErrorCorrection);
								dJointSetHinge2Param (jnt->_joint,dParamSuspensionCFM,jnt->suspensionForce);

								dJointSetHinge2Param (jnt->_joint,dParamFMax,jnt->maxTorque1);
								dJointSetHinge2Param (jnt->_joint,dParamLoStop,jnt->minAngle1);
								dJointSetHinge2Param (jnt->_joint,dParamHiStop,jnt->maxAngle1);
								dJointSetHinge2Param (jnt->_joint,dParamFudgeFactor,0.1);

								MNC(jnt);
							}
							//per-frame 
							//motor
							dJointSetHinge2Param (jnt->_joint,dParamVel2,jnt->desiredAngularVelocity2);
							dJointSetHinge2Param (jnt->_joint,dParamFMax2,jnt->maxTorque2);
							//steering
							avel = jnt->desiredAngularVelocity1;
							method_extension_axis1Angle = 0;
							if(method_extension_axis1Angle){
								//the specs don't have an axis1Angle on DoubleAxisHingeJoint - too bad
								//because here is how easy it would be to steer a car
								//instead, you have to chain a motor to axis1
								double agap = jnt->axis1Angle - dJointGetHinge2Angle1 (jnt->_joint);
								//printf("agap %lf = %f - %lf\n",agap,jnt->axis1Angle,dJointGetHinge2Angle1 (jnt->_joint));
								double avel = agap / .1 * jnt->desiredAngularVelocity1;
								if (agap > 0.1) avel = jnt->desiredAngularVelocity1;
								if (agap < -0.1) avel = -jnt->desiredAngularVelocity1;
							}
							dJointSetHinge2Param (jnt->_joint,dParamVel,avel);

						}
						break;
					case NODE_SliderJoint:
						{
							dJointID jointID;
							struct X3D_SliderJoint *jnt = (struct X3D_SliderJoint*)joint;
							if(!jnt->_joint){
								dBodyID body1ID, body2ID;
								struct X3D_RigidBody *xbody1, *xbody2;
								xbody1 = (struct X3D_RigidBody*)jnt->body1;
								xbody2 = (struct X3D_RigidBody*)jnt->body2;
								//allow for MULL body on one side of joint, to fix to static environment
								body1ID = xbody1 ? xbody1->_body : NULL;
								body2ID = xbody2 ? xbody2->_body : NULL;
								jointID = dJointCreateSlider (x3dworld->_world,x3dworld->_group);
								dJointAttach (jointID,body1ID,body2ID);
								jnt->_joint = jointID;
							}
							if(NNC(jnt)){
								float axislen = veclength3f(jnt->axis.c);
								if(axislen < .1){
									//specs say 0 0 0 is default but that's garbage, should be 0 0 1
									jnt->axis.c[0] = jnt->axis.c[1] = 0.0f;
									jnt->axis.c[2] = 1.0f;
								}
								if(!vecsame3f(jnt->__old_axis.c,jnt->axis.c)){
									dJointSetSliderAxis (jnt->_joint,jnt->axis.c[0],jnt->axis.c[1],jnt->axis.c[2]);
									veccopy3f(jnt->__old_axis.c,jnt->axis.c);
								}
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);
								dJointSetSliderParam (jnt->_joint,dParamBounce,jnt->stopBounce);
								dJointSetSliderParam (jnt->_joint,dParamStopERP,jnt->stopErrorCorrection);

								MNC(jnt);
							}
						}
						break;
					case NODE_UniversalJoint:
						{
							// http://ode.org/ode-latest-userguide.html#sec_7_3_4
							// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/rigid_physics.html#UniversalJoint
							dJointID jointID;
							struct X3D_UniversalJoint *jnt = (struct X3D_UniversalJoint*)joint;
							if(!jnt->_joint){
								dBodyID body1ID, body2ID;
								struct X3D_RigidBody *xbody1, *xbody2;
								xbody1 = (struct X3D_RigidBody*)jnt->body1;
								xbody2 = (struct X3D_RigidBody*)jnt->body2;
								//allow for MULL body on one side of joint, to fix to static environment
								body1ID = xbody1 ? xbody1->_body : NULL;
								body2ID = xbody2 ? xbody2->_body : NULL;
								jointID = dJointCreateUniversal (x3dworld->_world,x3dworld->_group);
								dJointAttach (jointID,body1ID,body2ID);
								jnt->_joint = jointID;
								//anchor point can be 0 0 0. If so, our vecsame3f technique below fails to set UniversalAnchor at least once.
								//so we do the zero-capable fields in the _joint section once
								dJointSetUniversalAnchor (jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
							}
							if(NNC(jnt)){
								float axislen;
								if(!vecsame3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c)){
									dJointSetUniversalAnchor (jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								}

								axislen = veclength3f(jnt->axis1.c);
								if(axislen < .1){
									//specs say 0 0 0 is default but that's garbage, should be 0 0 1
									jnt->axis1.c[0] = jnt->axis1.c[1] = 0.0f;
									jnt->axis1.c[2] = 1.0f;
								}
								axislen = veclength3f(jnt->axis2.c);
								if(axislen < .1){
									//specs say 0 0 0 is default but that's garbage, should be 0 0 1
									jnt->axis2.c[0] = jnt->axis2.c[2] = 0.0f;
									jnt->axis2.c[1] = 1.0f;
								}
								if(!vecsame3f(jnt->__old_axis1.c,jnt->axis1.c)){
									dJointSetUniversalAxis1 (jnt->_joint,jnt->axis1.c[0],jnt->axis1.c[1],jnt->axis1.c[2]);
									veccopy3f(jnt->__old_axis1.c,jnt->axis1.c);
								}
								if(!vecsame3f(jnt->__old_axis2.c,jnt->axis2.c)){
									dJointSetUniversalAxis2(jnt->_joint,jnt->axis2.c[0],jnt->axis2.c[1],jnt->axis2.c[2]);
									veccopy3f(jnt->__old_axis2.c,jnt->axis2.c);
								}
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);
								dJointSetUniversalParam (jnt->_joint,dParamBounce1,jnt->stop1Bounce);
								dJointSetUniversalParam (jnt->_joint,dParamStopERP1,jnt->stop1ErrorCorrection);
								dJointSetUniversalParam (jnt->_joint,dParamBounce2,jnt->stop2Bounce);
								dJointSetUniversalParam (jnt->_joint,dParamStopERP2,jnt->stop2ErrorCorrection);

								MNC(jnt);
							}
						}
						break;
					case NODE_MotorJoint:
						{
							// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/rigid_physics.html#MotorJoint
							// x3dmotorjoint only ode Angular AMotor, (no Linear LMotor parameters) 
							// only ode so-called User mode
							// http://ode.org/ode-latest-userguide.html#sec_7_5_1
							// table of joint parameters
							// dParamVel and dParamFMax relate specifically to AMotor joints
							dJointID jointID;
							struct X3D_MotorJoint *jnt = (struct X3D_MotorJoint*)joint;
							if(!jnt->_joint){
								dBodyID body1ID, body2ID;
								struct X3D_RigidBody *xbody1, *xbody2;
								xbody1 = (struct X3D_RigidBody*)jnt->body1;
								xbody2 = (struct X3D_RigidBody*)jnt->body2;
								//allow for MULL body on one side of joint, to fix to static environment
								body1ID = xbody1 ? xbody1->_body : NULL;
								body2ID = xbody2 ? xbody2->_body : NULL;
								jointID = dJointCreateAMotor (x3dworld->_world,x3dworld->_group);
								dJointAttach (jointID,body1ID,body2ID);
								jnt->_joint = jointID;
								dJointSetAMotorMode (jnt->_joint,dAMotorUser); 
								//dJointSetAMotorMode (jointID,dAMotorEuler);
							}
							if(NNC(jnt)){
								float axislen;
								dJointSetAMotorNumAxes (jnt->_joint,jnt->enabledAxes);
								//rel: relative to: 0-static 1-first body 2-2nd body
								if(jnt->enabledAxes >0 ){
									axislen = veclength3f(jnt->motor1Axis.c);
									if(axislen < .1){
										//specs say 0 0 0 is default but that's garbage, should be 0 0 1
										jnt->motor1Axis.c[0] = jnt->motor1Axis.c[1] = 0.0f;
										jnt->motor1Axis.c[2] = 1.0f;
									}
									
									if(!vecsame3f(jnt->__old_motor1Axis.c,jnt->motor1Axis.c)){
										dJointSetAMotorAxis (jnt->_joint,0,0, jnt->motor1Axis.c[0],jnt->motor1Axis.c[1],jnt->motor1Axis.c[2]);
										veccopy3f(jnt->__old_motor1Axis.c,jnt->motor1Axis.c);
									}
									if(jnt->__old_axis1Angle != jnt->axis1Angle){
										dJointSetAMotorAngle (jnt->_joint, 0, jnt->axis1Angle);
										jnt->__old_axis1Angle = jnt->axis1Angle;
									}
								}
								if(jnt->enabledAxes >1 ){
									axislen = veclength3f(jnt->motor2Axis.c);
									if(axislen < .1){
										//specs say 0 0 0 is default but that's garbage, should be 0 0 1
										jnt->motor2Axis.c[0] = jnt->motor2Axis.c[1] = 0.0f;
										jnt->motor2Axis.c[2] = 1.0f;
									}
									
									if(!vecsame3f(jnt->__old_motor2Axis.c,jnt->motor2Axis.c)){
										dJointSetAMotorAxis (jnt->_joint,1,1, jnt->motor2Axis.c[0],jnt->motor2Axis.c[1],jnt->motor2Axis.c[2]);
										veccopy3f(jnt->__old_motor2Axis.c,jnt->motor2Axis.c);
									}
									if(jnt->__old_axis2Angle != jnt->axis2Angle){
										dJointSetAMotorAngle (jnt->_joint, 1, jnt->axis2Angle);
										jnt->__old_axis2Angle = jnt->axis2Angle;
									}
									//dJointSetAMotorParam(jointID,dParamFMax2,jnt->axis2Torque);
								}
								if(jnt->enabledAxes >2 ){
									axislen = veclength3f(jnt->motor3Axis.c);
									if(axislen < .1){
										//specs say 0 0 0 is default but that's garbage, should be 0 0 1
										jnt->motor3Axis.c[0] = jnt->motor3Axis.c[1] = 0.0f;
										jnt->motor3Axis.c[2] = 1.0f;
									}
									
									if(!vecsame3f(jnt->__old_motor3Axis.c,jnt->motor3Axis.c)){
										dJointSetAMotorAxis (jnt->_joint,2,2, jnt->motor3Axis.c[0],jnt->motor3Axis.c[1],jnt->motor3Axis.c[2]);
										veccopy3f(jnt->__old_motor3Axis.c,jnt->motor3Axis.c);
									}
									if(jnt->__old_axis3Angle != jnt->axis3Angle){
										dJointSetAMotorAngle (jnt->_joint, 2, jnt->axis3Angle);
										jnt->__old_axis3Angle = jnt->axis3Angle;
									}
									//dJointSetAMotorParam(jointID,dParamFMax3,jnt->axis3Torque);
								}
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);

								dJointSetAMotorParam (jnt->_joint,dParamBounce1,jnt->stop1Bounce);
								dJointSetAMotorParam (jnt->_joint,dParamStopERP1,jnt->stop1ErrorCorrection);
								dJointSetAMotorParam (jnt->_joint,dParamBounce2,jnt->stop2Bounce);
								dJointSetAMotorParam (jnt->_joint,dParamStopERP2,jnt->stop2ErrorCorrection);
								dJointSetAMotorParam (jnt->_joint,dParamBounce3,jnt->stop3Bounce);
								dJointSetAMotorParam (jnt->_joint,dParamStopERP3,jnt->stop3ErrorCorrection);

								//addMotorTorques is a macro function in ODE that finds the bodies involve and
								//applies the torques to the bodies
								//if(jnt->autoCalc == TRUE)
									dJointAddAMotorTorques(jnt->_joint, jnt->axis1Torque, jnt->axis2Torque, jnt->axis3Torque);
								MNC(jnt);

							}
							//per-frame torque - this will cause an acceleration, don't know if that's 
							//what the x3dmotorjoint torque fields were meant for
							//dJointAddAMotorTorques(jnt->_joint, jnt->axis1Torque, jnt->axis2Torque, jnt->axis3Torque);
							if(jnt->autoCalc == FALSE){
								//user sets angles on each frame
								if(jnt->enabledAxes >0)
									dJointSetAMotorAngle (jnt->_joint, 0, jnt->axis1Angle);
								if(jnt->enabledAxes >1)
									dJointSetAMotorAngle (jnt->_joint, 1, jnt->axis2Angle);
								if(jnt->enabledAxes >2)
									dJointSetAMotorAngle (jnt->_joint, 2, jnt->axis3Angle);
							}
						}
						break;
					default:
						break;
				} //switch (joint type)
			}

			//RUN PHYSICS ENGINE
			dSpaceCollide (space,0,&nearCallback);
			if (!pause) {
				if(x3dworld->preferAccuracy){
					//PUNISHING GIANT MATRIX PUSHED ONTO C STACK FOR GIANT MATRIX SOLUTION
					double step_fraction = STEP_SIZE;
					dWorldStep (x3dworld->_world,step_fraction); 
				}else{
					//SO CALLED QUICK-STEP METHOD WHICH IS FASTER, THE NORMAL WAY TO DO IT
					//dWorldSetQuickStepNumIterations (x3dworld->_world, x3dworld->iterations);
					double step_fraction = STEP_SIZE / (double)x3dworld->iterations;
					for(nstep=0;nstep<x3dworld->iterations;nstep++)
						dWorldQuickStep (x3dworld->_world,step_fraction); 
				}
			}

			//do eventOuts
			//Rigidbody -> Collidable
			for(i=0;i<x3dworld->bodies.n;i++){
				x3dbody = (struct X3D_RigidBody*)x3dworld->bodies.p[i];
				if(x3dbody->_body){
					//if not fixed, it will have a body that maybe moved
					//ATTEMPT 5 and 6
					//we set and mark both x3dbody and top-level collidable translation,rotation
					//top level collidable: we concatonate x3dboy * top-collidable transform
					const dReal *dpos, *dquat;
					//dReal *drot;
					Quaternion quat;
					double xyza[4];

					dpos = dBodyGetPosition (x3dbody->_body);
					//printf("dpos = %lf %lf %lf\n",dpos[0],dpos[1],dpos[2]);
					dquat = dBodyGetQuaternion(x3dbody->_body);
					quat.x = dquat[1], quat.y = dquat[2], quat.z = dquat[3], quat.w = dquat[0];
					quaternion_to_vrmlrot(&quat,&xyza[0],&xyza[1],&xyza[2],&xyza[3]);
					double2float(x3dbody->position.c,dpos,3);
					double2float(x3dbody->orientation.c,xyza,4);
					MARK_EVENT(X3D_NODE(x3doffset),offsetof(struct X3D_RigidBody,position));
					MARK_EVENT(X3D_NODE(x3doffset),offsetof(struct X3D_RigidBody,orientation));

					for(k=0;k<x3dbody->geometry.n;k++){
						float *translation, *rotation;
						struct X3D_CollidableOffset* x3doffset = (struct X3D_CollidableOffset*)x3dbody->geometry.p[k];
						translation = x3doffset->translation.c;
						rotation = x3doffset->rotation.c;
						//ATTEMPT 5: concatonate rigidbody transform with top-level collidable transform
						if(x3doffset->_nodeType == NODE_CollidableOffset){
							//body_T * body_R * geom_T * geom_R ==
							//(body_T + body_R*geom_T) * body_R*geom R
							double geomT[3], geomR[4];
							Quaternion geomQ;
							float2double(geomT,x3doffset->_initialTranslation.c,3);
							float2double(geomR,x3doffset->_initialRotation.c,4);
							vrmlrot_to_quaternion(&geomQ,geomR[0],geomR[1],geomR[2],geomR[3]);
							quaternion_rotationd(geomT,&quat,geomT);
							double2float(translation,geomT,3);
							vecadd3f(translation,translation,x3dbody->position.c);
							quaternion_multiply(&geomQ,&quat,&geomQ);
							quaternion_to_vrmlrot(&geomQ,&geomR[0],&geomR[1],&geomR[2],&geomR[3]);
							double2float(rotation,geomR,4);
						}
						if(x3doffset->_nodeType == NODE_CollidableShape){
							veccopy3f(translation,x3dbody->position.c);
							veccopy4f(rotation,x3dbody->orientation.c);
						}
						//double2float(translation,dpos,3);
						//double2float(rotation,xyza,4);
						if(0) if(i==0){
							//some debug 
							static int loopcount = 0;
							if(loopcount < 30){
								printf("pos %lf %lf %lf\n",dpos[0],dpos[1],dpos[2]);
								printf("rot %f %f %f %f\n",rotation[0],rotation[1],rotation[2],rotation[3]);
								printf("trn %f %f %f\n",translation[0],translation[1],translation[2]);
								loopcount++;
							}
						}
						//if(1) printf("transout %f %f %f\n",translation[0],translation[1],translation[2]);
						MARK_EVENT(X3D_NODE(x3doffset),offsetof(struct X3D_CollidableOffset,translation));
						MARK_EVENT(X3D_NODE(x3doffset),offsetof(struct X3D_CollidableOffset,rotation));

						x3doffset->_change++;
					}
				}
			} //for bodies
			for(i=0;i<x3dworld->joints.n;i++){
				struct X3D_Node *joint = (struct X3D_Node*)x3dworld->joints.p[i];
				// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/rigid_physics.html#X3DRigidJointNode
				// 	forceOutput = [NONE] ["NONE","ALL", specific output field name ie for MotorJoint "motor1AngleRate"
				switch(joint->_nodeType){
					case NODE_BallJoint:
						{
							//dJointID jointID;
							struct X3D_BallJoint *jnt = (struct X3D_BallJoint*)joint;
							if(jnt->_forceout){
								if(jnt->_forceout & FORCEOUT_body1AnchorPoint){
							//		jnt->body1AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_BallJoint,body1AnchorPoint));
								}
								if(jnt->_forceout & FORCEOUT_body2AnchorPoint){
							//		jnt->body2AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_BallJoint,body2AnchorPoint));
								}

							}
						}
						break;
					case NODE_SingleAxisHingeJoint:
						{
							//dJointID jointID;
							struct X3D_SingleAxisHingeJoint *jnt = (struct X3D_SingleAxisHingeJoint*)joint;
							if(jnt->_forceout){

  								if(jnt->_forceout & FORCEOUT_angle){
							//		jnt->angle = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_SingleAxisHingeJoint,angle));
								}
								if(jnt->_forceout & FORCEOUT_angleRate){
							//		jnt->angleRate = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_SingleAxisHingeJoint,angleRate));
								}

  								if(jnt->_forceout & FORCEOUT_body1AnchorPoint){
							//		jnt->body1AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_SingleAxisHingeJoint,body1AnchorPoint));
								}
								if(jnt->_forceout & FORCEOUT_body2AnchorPoint){
							//		jnt->body2AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_SingleAxisHingeJoint,body2AnchorPoint));
								}
							}
						}
						break;
					case NODE_DoubleAxisHingeJoint:
						{
							//dJointID jointID;
							struct X3D_DoubleAxisHingeJoint *jnt = (struct X3D_DoubleAxisHingeJoint*)joint;
							if(jnt->_forceout){
								if(jnt->_forceout & FORCEOUT_body1AnchorPoint){
							//		jnt->body1AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,body1AnchorPoint));
								}
								if(jnt->_forceout & FORCEOUT_body1Axis){
							//		jnt->body1Axis = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,body1Axis));
								}
								if(jnt->_forceout & FORCEOUT_body2AnchorPoint){
							//		jnt->body2AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,body2AnchorPoint));
								}
								if(jnt->_forceout & FORCEOUT_body2Axis){
							//		jnt->body2Axis = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,body2Axis));
								}
								if(jnt->_forceout & FORCEOUT_hinge1Angle){
							//		jnt->hinge1Angle = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,hinge1Angle));
								}
								if(jnt->_forceout & FORCEOUT_hinge1AngleRate){
							//		jnt->hinge1AngleRate = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,hinge1AngleRate));
								}
								if(jnt->_forceout & FORCEOUT_hinge2Angle){
							//		jnt->hinge2Angle = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,hinge2Angle));
								}
								if(jnt->_forceout & FORCEOUT_hinge2AngleRate){
							//		jnt->hinge2AngleRate = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_DoubleAxisHingeJoint,hinge2AngleRate));
								}

							}
						}
						break;
					case NODE_SliderJoint:
						{
							//dJointID jointID;
							struct X3D_SliderJoint *jnt = (struct X3D_SliderJoint*)joint;
  							if(jnt->_forceout){
  								if(jnt->_forceout & FORCEOUT_separation){
							//		jnt->separation = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_SliderJoint,separation));
								}
								if(jnt->_forceout & FORCEOUT_separationRate){
							//		jnt->separationRate = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_SliderJoint,separationRate));
								}
							}
						}
						break;
					case NODE_UniversalJoint:
						{
							//dJointID jointID;
							struct X3D_UniversalJoint *jnt = (struct X3D_UniversalJoint*)joint;
  							if(jnt->_forceout){
								if(jnt->_forceout & FORCEOUT_body1AnchorPoint){
							//		jnt->body1AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_UniversalJoint,body1AnchorPoint));
								}
								if(jnt->_forceout & FORCEOUT_body1Axis){
							//		jnt->body1Axis = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_UniversalJoint,body1Axis));
								}
								if(jnt->_forceout & FORCEOUT_body2AnchorPoint){
							//		jnt->body2AnchorPoint = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_UniversalJoint,body2AnchorPoint));
								}
								if(jnt->_forceout & FORCEOUT_body2Axis){
							//		jnt->body2Axis = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_UniversalJoint,body2Axis));
								}
							}
						}
						break;
					case NODE_MotorJoint:
						{
							//dJointID jointID;
							struct X3D_MotorJoint *jnt = (struct X3D_MotorJoint*)joint;
							if(jnt->_forceout){
								if(jnt->_forceout & FORCEOUT_motor1Angle){
									jnt->motor1Angle = (float) dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor1Angle));
								}
								if(jnt->_forceout & FORCEOUT_motor1AngleRate){
									jnt->motor1AngleRate = (float) dJointGetAMotorAngleRate( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor1AngleRate));
								}
								if(jnt->_forceout & FORCEOUT_motor2Angle){
									jnt->motor2Angle = (float) dJointGetAMotorAngle( jnt->_joint, 1 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor2Angle));
								}
								if(jnt->_forceout & FORCEOUT_motor2AngleRate){
									jnt->motor2AngleRate = (float) dJointGetAMotorAngleRate( jnt->_joint, 1 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor2AngleRate));
								}
								if(jnt->_forceout & FORCEOUT_motor3Angle){
									jnt->motor3Angle = (float) dJointGetAMotorAngle( jnt->_joint, 2 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor3Angle));
								}
								if(jnt->_forceout & FORCEOUT_motor3AngleRate){
									jnt->motor3AngleRate = (float) dJointGetAMotorAngleRate( jnt->_joint, 2 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor3AngleRate));
								}
							}

						}
						break;
					default:
						break;
				} //switch on joint
			} //for joints
			// remove all contact joints
			dJointGroupEmpty (contactgroup);
		}
	}

}

void register_CollisionCollection(struct X3D_Node * _node){
	if(_node->_nodeType == NODE_CollisionCollection){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollisionSensor *node = (struct X3D_CollisionSensor*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
		if(!x3dcollisioncollections) x3dcollisioncollections = newVector(struct X3D_Node*,5);
		if(!space) dHashSpaceCreate(0); //default space
		vector_pushBack(struct X3D_Node*,x3dcollisioncollections,_node);
		MARK_NODE_COMPILED;
	}
}
void register_CollisionSensor(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_CollisionSensor){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollisionSensor *node = (struct X3D_CollisionSensor*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
		if(!x3dcollisionsensors) x3dcollisionsensors = newVector(struct X3D_Node*,5);
		vector_pushBack(struct X3D_Node*,x3dcollisionsensors,_node);
		MARK_NODE_COMPILED;
	}
}

void register_RigidBodyCollection(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_RigidBodyCollection){
		ppComponent_RigidBodyPhysics p;
		dJointGroupID groupID;
		struct X3D_RigidBodyCollection *node = (struct X3D_RigidBodyCollection*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;

		if(!space)
			space = dHashSpaceCreate (0); //default space
		if(!world){
			world = dWorldCreate();
			dWorldSetGravity (world,node->gravity.c[0],node->gravity.c[1],node->gravity.c[2]);
			dWorldSetCFM (world,1e-5);
			dWorldSetAutoDisableFlag (world,1);
			//this contactgroup is a default group for doing just collisions, cleared every frame:
			// 1)collide (callback to make temporary joints out of contact points) 
			// 2)physics step (uses all joints)
			// 3)clear contactgroup (clean out all the temporary collision joints)
			contactgroup = dJointGroupCreate (0); 

			#if 1

			dWorldSetAutoDisableAverageSamplesCount( world, 10 );

			#endif

			dWorldSetLinearDamping(world, 0.00001);
			dWorldSetAngularDamping(world, 0.005);
			dWorldSetMaxAngularSpeed(world, 200);

			dWorldSetContactMaxCorrectingVel (world,0.1);
			dWorldSetContactSurfaceLayer (world,0.001);
		} 
		///// we assign above node->_world = (void*)world;
		//node->_space = (void*)space;
		groupID = dJointGroupCreate (0); //this is a contact group for joints
		node->_group = groupID;
		if(!x3dworlds) x3dworlds = newVector(struct X3D_Node*,5);
		vector_pushBack(struct X3D_Node*,x3dworlds,_node);
		MARK_NODE_COMPILED;
	}
}

// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/rigid_physics.html#TransformationHierarchy
// "Nodes .. are not part of the transform hierarchy.."
// 37.4.3 CollidableShape: "When placed under a part of the transformation hierarchy, 
// it can be used to visually represent the movement of the object."
// Interpretation:
// if/when put outside of the X3DRigidBodyCollection -via DEF / USE- then 
// the collidableshape (and any wrapper collidableOffset) transforms are pushed onto the 
// render_hier transform stack and the (collision/physics proxy) Shape is rendered
// (versus if hidden with Switch -1, or only DEFed inside X3DRigidBodyCollection, Shape is never
// rendered, but CollidableShape and CollidableOffset should output translation_changed,
// and rotation_changed events, so can route to transforms containing non-proxy shapes

// option: just expose compile and render virt_ functions for collidableshape and collidableoffset
//- and handle prep/fin/child privately


void prep_CollidableShape(struct X3D_Node *_node){
	struct X3D_CollidableShape *node = (struct X3D_CollidableShape*)_node;
	if(node){
		COMPILE_IF_REQUIRED

		//CollidableOffset will come in here too, so in perl, keep the first fields in same order 
		//so we can caste to one, and the fields jive
		if(!renderstate()->render_vp) {

				/* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
				 * so we do nothing here in that case -ncoder */

			/* printf ("prep_Transform, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
			 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

			/* do we have any geometry visible, and are we doing anything with geometry? */
			OCCLUSIONTEST

			if(!renderstate()->render_vp) {
				/* do we actually have any thing to rotate/translate/scale?? */

				FW_GL_PUSH_MATRIX();

				/* TRANSLATION */
				if (node->__do_trans)
					FW_GL_TRANSLATE_F(node->translation.c[0],node->translation.c[1],node->translation.c[2]);

				/* ROTATION */
				if (node->__do_rotation) {
					FW_GL_ROTATE_RADIANS(node->rotation.c[3], node->rotation.c[0],node->rotation.c[1],node->rotation.c[2]);
				}
				RECORD_DISTANCE
			}
		}
	}
}
void compile_CollidableShape(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_CollidableShape || _node->_nodeType == NODE_CollidableOffset){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollidableShape *node = (struct X3D_CollidableShape*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
		
		INITIALIZE_EXTENT;
		node->__do_trans = verify_translate ((GLfloat *)node->translation.c);
		node->__do_rotation = verify_rotate ((GLfloat *)node->rotation.c);
		MARK_NODE_COMPILED;
	}
}

void fin_CollidableShape(struct X3D_Node *_node){
	if(_node){
        if(!renderstate()->render_vp) {
				FW_GL_POP_MATRIX();
        } else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if((_node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) {
				struct X3D_CollidableShape *node = (struct X3D_CollidableShape*)_node;

                FW_GL_ROTATE_RADIANS(-(((node->rotation).c[3])),((node->rotation).c[0]),((node->rotation).c[1]),((node->rotation).c[2])
                );
                FW_GL_TRANSLATE_F(-(((node->translation).c[0])),-(((node->translation).c[1])),-(((node->translation).c[2]))
                );
            }
        }
	}
}
void compile_CollidableOffset(struct X3D_Node *node){
	compile_CollidableShape(node);
}
void child_CollidableShape(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_CollidableShape){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollidableShape *node = (struct X3D_CollidableShape*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
		if(node->shape) render_node(node->shape);
	}
}
void prep_CollidableOffset(struct X3D_Node *node){
	prep_CollidableShape(node);
}
void fin_CollidableOffset(struct X3D_Node *node){
	fin_CollidableShape(node);
}
void child_CollidableOffset(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_CollidableOffset){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollidableOffset *node = (struct X3D_CollidableOffset*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;

		//->collidable is an SFNode. Options:
		// a) we could go through normalChildren(n=1,p=&collidable) which would call prep and fin and child for us
		// b) we can call prep_, child_ and fin_ ourselves
		if(node->collidable) {
			switch(node->collidable->_nodeType){
				case NODE_CollidableOffset:
					prep_CollidableOffset(node->collidable);
					child_CollidableOffset(node->collidable);
					fin_CollidableOffset(node->collidable);
					break;
				case NODE_CollidableShape:
					prep_CollidableShape(node->collidable);
					child_CollidableShape(node->collidable);
					fin_CollidableShape(node->collidable);
					break;
				default:
					break;
			}
		}
	}
}

void do_CollisionSensorTick0(struct X3D_CollisionSensor *node){
	//if we call this from the routing do_...Tick
	//that happens before rbp_run_phsyics() is called
	//so in rbp_run_physics the first thing we can do is clear the contacts in any sensors
	//then any contacts generated during physics can come out of here in the next routing session
	if(!static_did_physics_since_last_Tick) return;
	static_did_physics_since_last_Tick = 0;
	if(!node->enabled) return;

	if(node->contacts.n){
		if(node->isActive == FALSE){
			node->isActive = TRUE;
			MARK_EVENT(X3D_NODE(node),offsetof(struct X3D_CollisionSensor,isActive));
		}
		MARK_EVENT(X3D_NODE(node),offsetof(struct X3D_CollisionSensor,contacts));
		//node->contacts.n = 0;
		// leave at 100 FREE_IF_NZ(node->contacts.p);
	}else{
		if(node->isActive == TRUE){
			node->isActive = FALSE;
			MARK_EVENT(X3D_NODE(node),offsetof(struct X3D_CollisionSensor,isActive));
			//do I need to route n=0? 
			//leave at 100 node->contacts.p = NULL; //if I don't set to null and mark, then CRoutes bombs in some cleanup code.
			//MARK_EVENT(X3D_NODE(node),offsetof(struct X3D_CollisionSensor,contacts));
		}
	}
	if(node->isActive){
		if(node->intersections.n){
			MARK_EVENT(X3D_NODE(node),offsetof(struct X3D_CollisionSensor,intersections));
			//node->intersections.n = 0;
			//leave at 100 FREE_IF_NZ(node->intersections.p);
		}
		//else{
		//	MARK_EVENT(X3D_NODE(node),offsetof(struct X3D_CollisionSensor,intersections));
		//}
	}
}
void do_CollisionSensorTick(void * ptr){
	if(ptr)
		do_CollisionSensorTick0((struct X3D_CollisionSensor *)ptr);
}

void add_physics(struct X3D_Node *node){
	switch(node->_nodeType){
		case NODE_CollisionSensor:
			//Nov. 2016: H: this should be in do_first ie do_CollisionSensor
			//which gets called before routing ie so if there's a Contact it can be routed
			//its almost the same place as physics, which is just after do_events,routing
			// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/concepts.html#ExecutionModel
			register_CollisionSensor(node);
			break;
		case NODE_CollisionCollection:
			//CollisionCollections are x3dchild nodes, and can appear naked in scenegraph
			//or in CollisionSensor field, or in RigidBodyCollection field
			//ie might be DEF/USED
			register_CollisionCollection(node);
			break;
		case NODE_RigidBodyCollection:
			//OK good.
			register_RigidBodyCollection(node);
			break;
		//Q. what about CollisionSpace, CollisionCollection - should they be registered here?
		default:
			break;
	}
}

//END MIT LIC <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#else //else no ode phyiscs engine, just stubs

void compile_CollidableShape(struct X3D_Node *node){
}
void prep_CollidableShape(struct X3D_Node *node){
}
void fin_CollidableShape(struct X3D_Node *node){
}
void child_CollidableShape(struct X3D_Node *node){
}
void compile_CollidableOffset(struct X3D_Node *node){
}
void prep_CollidableOffset(struct X3D_Node *node){
}
void fin_CollidableOffset(struct X3D_Node *node){
}
void child_CollidableOffset(struct X3D_Node *node){
}
void rbp_run_physics(){
}
void add_physics(struct X3D_Node *node){
}
void do_CollisionSensorTick(void * ptr){
}

#endif	
	
	
	
	
