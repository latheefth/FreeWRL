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

void rbp_run_physics();
void set_physics();

//#undef WITH_RBP
#ifdef WITH_RBP
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
CollidableOffset	GeomSetPosition,Rotation,Quat	
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
static dJointGroupID contactgroup;
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

// this is called by dSpaceCollide when two objects in space are
// potentially colliding.
static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
  int i, numc;
  dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per box-box
  // if (o1->body && o2->body) return;

  // exit without doing anything if the two bodies are connected by a joint
  dBodyID b1 = dGeomGetBody(o1);
  dBodyID b2 = dGeomGetBody(o2);
  if (b1 && b2 && dAreConnectedExcluding (b1,b2,dJointTypeContact)) return;

  for (i=0; i<MAX_CONTACTS; i++) {
    contact[i].surface.mode = dContactBounce; // | dContactSoftCFM;
    contact[i].surface.mu = .1; //dInfinity;
    contact[i].surface.mu2 = 0;
    contact[i].surface.bounce = .2;
    contact[i].surface.bounce_vel = 0.1;
    contact[i].surface.soft_cfm = 0.01;
  }
  if (numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom,
			   sizeof(dContact))) {
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
    }
  }
}

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

static int init_rbp_once = 0;
static dThreadingImplementationID threading;
static dThreadingThreadPoolID pool;
int init_rbp(){
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
		struct force_output_fieldname *fname;
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

void rbp_run_physics(){
	/*	called once per frame.
		assumes we have lists of registered worlds (x3drigidbodycollections) and 
			registered collisionSensors if any
		for each world, 
			- we drill down and check each thing and if not initialized, we initialize it 
			- we run the collision and simulation steps
			- we output to any collisionSensors
	*/
	ppComponent_RigidBodyPhysics p;
	p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	
	int nstep = 1;
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

	if(init_rbp()){
		int i,j,k;
		struct X3D_RigidBodyCollection *x3dworld;
		for(j=0;j<vectorSize(x3dworlds);j++){
			struct X3D_RigidBody *x3dbody;
			struct X3D_CollidableOffset *x3doffset;
			struct X3D_CollidableShape *x3dcshape;
			x3dworld = (struct X3D_RigidBodyCollection*)vector_get(struct X3D_Node*,x3dworlds,j);
			//Collidable -> rigidbody 

			//update bodies
			for(i=0;i<x3dworld->bodies.n;i++){
				x3dbody = (struct X3D_RigidBody*)x3dworld->bodies.p[i];
				if(!x3dbody->_body && !x3dbody->fixed){
					x3dbody->_body = dBodyCreate (x3dworld->_world);
					dBodySetData (x3dbody->_body,(void*) x3dbody);
				}
				x3dcshape = NULL;
				x3doffset = NULL;
				for(k=0;k<x3dbody->geometry.n;k++){
					struct SFVec3f translation;
					struct SFRotation rotation;
					if(x3dbody->geometry.p[k]->_nodeType == NODE_CollidableOffset){
						x3doffset = (struct X3D_CollidableOffset*)x3dbody->geometry.p[k];
						x3dcshape = (struct X3D_CollidableShape*)x3doffset->collidable;
					}else if(x3dbody->geometry.p[k]->_nodeType == NODE_CollidableShape){
						x3dcshape = (struct X3D_CollidableShape*)x3dbody->geometry.p[k];
						x3doffset = (struct X3D_CollidableOffset*)x3dcshape;
					}
					if(!x3dcshape->_geom){
						dGeomID gid = NULL;
						struct X3D_Shape *shape = (struct X3D_Shape*)x3dcshape->shape;
						if(shape && shape->geometry){
							dReal sides[3];
							switch(shape->geometry->_nodeType){
								case NODE_Box:
									{
										struct X3D_Box *box = (struct X3D_Box*)shape->geometry;
										sides[0] = box->size.c[0]; sides[1] = box->size.c[1], sides[2] = box->size.c[2];
										gid = dCreateBox(x3dworld->_space,sides[0],sides[1],sides[2]);
									}
									break;
								case NODE_Cylinder:
									{
										struct X3D_Cylinder *cyl = (struct X3D_Cylinder*)shape->geometry;
										sides[0] = cyl->radius;
										sides[1] = cyl->height;
										gid = dCreateCylinder(x3dworld->_space,sides[0],sides[1]);
									}
									break;
								//case convex - not done yet, basically indexedfaceset
								case NODE_Sphere:
								default:
									{
										struct X3D_Sphere *sphere = (struct X3D_Sphere*)shape->geometry;
										sides[0] = sphere->radius;
										gid = dCreateSphere(x3dworld->_space,sides[0]);
									}
									break;
							}
							x3dcshape->_geom = gid;
							MNC(x3dcshape);
						}
						//for a fixed body, use the body position to position the geometry
						translation = x3dbody->position;
						rotation = x3dbody->orientation;
						//if(FALSE || !x3dbody->fixed){
						if(x3dbody->_body){
							dGeomSetBody(x3dcshape->_geom,x3dbody->_body);
						}
						dGeomSetPosition (x3dcshape->_geom, (dReal)translation.c[0],(dReal)translation.c[1],(dReal)translation.c[2]);
						if(1){
							dReal dquat[4];
							Quaternion quat;
							vrmlrot_to_quaternion(&quat,rotation.c[0],rotation.c[1],rotation.c[2],rotation.c[3]);
							dquat[0] = quat.w; dquat[1] = quat.x, dquat[2] = quat.y, dquat[3] = quat.z;
							dGeomSetQuaternion(x3dcshape->_geom,dquat);
						}else{
							dMatrix3 R;
							dRFromAxisAndAngle (R,rotation.c[0],rotation.c[1],rotation.c[2],rotation.c[3]);
							dGeomSetRotation(x3dcshape->_geom,R);
						}
						if(x3dbody->autoDamp)
							dBodySetAngularDamping(x3dbody->_body, x3dbody->angularDampingFactor);
						else
							dBodySetAngularDamping(x3dbody->_body, 0.0);
							
					} //if !geom

				} //geometries

				if(x3dbody->_body){
					//not fixed
					if(NNC(x3dbody)){
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
								default:
									{
										struct X3D_Sphere *sphere = (struct X3D_Sphere*)x3dbody->massDensityModel;
										sides[0] = sphere->radius;
										dMassSetSphere (&m,DENSITY,sides[0]);
									}
									break;
							}
							dMassAdjust(&m,x3dbody->mass);
							dBodySetMass (x3dbody->_body, &m);
						}else{
							dMass m;
							dMassSetSphere (&m,DENSITY,.01);
							dMassAdjust(&m,x3dbody->mass);
							dBodySetMass (x3dbody->_body, &m);
						}

						if(x3dbody->useFiniteRotation){
							if(!vecsame3f(x3dbody->__old_finiteRotationAxis.c,x3dbody->finiteRotationAxis.c)){
								dBodySetFiniteRotationAxis (x3dbody->_body, x3dbody->finiteRotationAxis.c[0],x3dbody->finiteRotationAxis.c[1],x3dbody->finiteRotationAxis.c[2]);
								veccopy3f(x3dbody->__old_finiteRotationAxis.c,x3dbody->finiteRotationAxis.c);
							}
						}

						//gravity?
						if(!x3dbody->useGlobalGravity){
							dBodySetGravityMode(x3dbody->_body,0);
						}
						//position and orientation set once in if(!_geom) above
						//add any per-step per-body forces
						float speed = veclength3f(x3dbody->linearVelocity.c);
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
					} //if NCC(x3dbody)
					if(x3dbody->autoDamp){
					}
					if(x3dbody->forces.n){
					}
					if(x3dbody->torques.n){
					}
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
								veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								veccopy3f(jnt->__old_axis.c,jnt->axis.c);
							}
							if(NNC(jnt)){
								if(!vecsame3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c)){
									dJointSetHingeAnchor (jointID,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								}
								float axislen = veclength3f(jnt->axis.c);
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
								MNC(jnt);
							}
						}
						break;
					case NODE_DoubleAxisHingeJoint:
						{
							static int useD = FALSE; //not sure what DHinge is. Hinge2 matches x3d specs better.
							dJointID jointID;
							struct X3D_DoubleAxisHingeJoint *jnt = (struct X3D_DoubleAxisHingeJoint*)joint;
							if(!jnt->_joint){
								dBodyID body1ID, body2ID;
								struct X3D_RigidBody *xbody1, *xbody2;
								xbody1 = (struct X3D_RigidBody*)jnt->body1;
								xbody2 = (struct X3D_RigidBody*)jnt->body2;
								//allow for MULL body on one side of joint, to fix to static environment
								body1ID = xbody1 ? xbody1->_body : NULL;
								body2ID = xbody2 ? xbody2->_body : NULL;
								if(useD)
									jointID = dJointCreateDHinge (x3dworld->_world,x3dworld->_group);
								else
									jointID = dJointCreateHinge2 (x3dworld->_world,x3dworld->_group);
								dJointAttach (jointID,body1ID,body2ID);
								jnt->_joint = jointID;
								if(useD){
									dJointSetDHingeAnchor1(jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									float anchor2[3];
									vecadd3f(anchor2,jnt->anchorPoint.c,jnt->axis2.c);
									dJointSetDHingeAnchor2(jnt->_joint, anchor2[0],anchor2[1],anchor2[2]);
									dJointSetDHingeAxis(jnt->_joint,jnt->axis1.c[0],jnt->axis1.c[1],jnt->axis1.c[2]);
								}else{
									dJointSetHinge2Anchor(jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									dJointSetHinge2Axis1(jnt->_joint,jnt->axis1.c[0],jnt->axis1.c[1],jnt->axis1.c[2]);
									dJointSetHinge2Axis2(jnt->_joint,jnt->axis2.c[0],jnt->axis2.c[1],jnt->axis2.c[2]);
								}
							}
							if(NNC(jnt)){
								float axislen = veclength3f(jnt->axis1.c);
								if(axislen < .1){
									//specs say 0 0 0 is default but that's garbage, should be 0 0 1
									jnt->axis1.c[0] = jnt->axis1.c[1] = 0.0f;
									jnt->axis1.c[2] = 1.0f;
								}
								if(!vecsame3f(jnt->__old_axis1.c,jnt->axis1.c)){
									if(useD)
										dJointSetDHingeAxis(jnt->_joint,jnt->axis1.c[0],jnt->axis1.c[1],jnt->axis1.c[2]);
									else
										dJointSetHinge2Axis1(jnt->_joint,jnt->axis1.c[0],jnt->axis1.c[1],jnt->axis1.c[2]);
									veccopy3f(jnt->__old_axis1.c,jnt->axis1.c);
								}
								if(!vecsame3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c)){
									if(useD)
										dJointSetDHingeAnchor1(jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									else
										dJointSetHinge2Anchor(jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								}
								if(!vecsame3f(jnt->__old_axis2.c,jnt->axis2.c)){
									if(useD){
										float anchor2[3];
										vecadd3f(anchor2,jnt->anchorPoint.c,jnt->axis2.c);
										dJointSetDHingeAnchor2(jnt->_joint, anchor2[0],anchor2[1],anchor2[2]);
									}else{
										dJointSetHinge2Axis2(jnt->_joint,jnt->axis2.c[0],jnt->axis2.c[1],jnt->axis2.c[2]);
									}
									veccopy3f(jnt->__old_axis2.c,jnt->axis2.c);
								}
								jnt->_forceout = forceout_from_names(jnt->forceOutput.n,jnt->forceOutput.p);
								MNC(jnt);
							}
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
								if(!vecsame3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c)){
									dJointSetUniversalAnchor (jnt->_joint,jnt->anchorPoint.c[0],jnt->anchorPoint.c[1],jnt->anchorPoint.c[2]);
									veccopy3f(jnt->__old_anchorPoint.c,jnt->anchorPoint.c);
								}

								float axislen = veclength3f(jnt->axis1.c);
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
								if(TRUE || jnt->enabledAxes >0 ){
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
								if(TRUE || jnt->enabledAxes >1 ){
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
								if(TRUE || jnt->enabledAxes >2 ){
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

								//addMotorTorques is a macro function in ODE that finds the bodies involve and
								//applies the torques to the bodies
								dJointAddAMotorTorques(jnt->_joint, jnt->axis1Torque, jnt->axis2Torque, jnt->axis3Torque);
								MNC(jnt);

							}
							//per-frame torque - this will cause an acceleration, don't know if that's 
							//what the x3dmotorjoint torque fields were meant for
							//dJointAddAMotorTorques(jnt->_joint, jnt->axis1Torque, jnt->axis2Torque, jnt->axis3Torque);
						}
						break;
					default:
						break;
				} //switch (joint type)
			}


			//RUN PHYSICS ENGINE
			dSpaceCollide (x3dworld->_space,0,&nearCallback);
			if (!pause) {
				double step_fraction = STEP_SIZE / (double)x3dworld->iterations;
				for(int kstep=0;kstep<nstep*x3dworld->iterations;kstep++)
					dWorldQuickStep (x3dworld->_world,step_fraction); //STEP_SIZE); //0.02);
			}

			//do eventOuts
			//Rigidbody -> Collidable
			for(i=0;i<x3dworld->bodies.n;i++){
				x3dbody = (struct X3D_RigidBody*)x3dworld->bodies.p[i];
				if(x3dbody->_body){
					//if not fixed, it will have a body that maybe moved
					x3dcshape = NULL;
					x3doffset = NULL;
					for(k=0;k<x3dbody->geometry.n;k++){
						struct SFVec3f translation;
						struct SFRotation rotation;
						dReal *dpos, *dquat;
						Quaternion quat;
						double xyza[4];

						if(x3dbody->geometry.p[k]->_nodeType == NODE_CollidableOffset){
							x3doffset = (struct X3D_CollidableOffset*)x3dbody->geometry.p[k];
							x3dcshape = (struct X3D_CollidableShape*)x3doffset->collidable;
						}else if(x3dbody->geometry.p[k]->_nodeType == NODE_CollidableShape){
							x3dcshape = (struct X3D_CollidableShape*)x3dbody->geometry.p[k];
							x3doffset = (struct X3D_CollidableOffset*)x3dcshape;
						}
						translation = x3doffset->translation;
						rotation = x3doffset->rotation;
						dpos = dBodyGetPosition (x3dbody->_body);
						dquat = dBodyGetQuaternion(x3dbody->_body);
						quat.x = dquat[1], quat.y = dquat[2], quat.z = dquat[3], quat.w = dquat[0];
						quaternion_to_vrmlrot(&quat,&xyza[0],&xyza[1],&xyza[2],&xyza[3]);

						double2float(x3doffset->translation.c,dpos,3);
						double2float(x3doffset->rotation.c,xyza,4);
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
							dJointID jointID;
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
							dJointID jointID;
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
							dJointID jointID;
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
							dJointID jointID;
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
							dJointID jointID;
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
							dJointID jointID;
							struct X3D_MotorJoint *jnt = (struct X3D_MotorJoint*)joint;
							if(jnt->_forceout){
								if(jnt->_forceout & FORCEOUT_motor1Angle){
									jnt->motor1Angle = dJointGetAMotorAngle( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor1Angle));
								}
								if(jnt->_forceout & FORCEOUT_motor1AngleRate){
									jnt->motor1AngleRate = dJointGetAMotorAngleRate( jnt->_joint, 0 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor1AngleRate));
								}
								if(jnt->_forceout & FORCEOUT_motor2Angle){
									jnt->motor2Angle = dJointGetAMotorAngle( jnt->_joint, 1 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor2Angle));
								}
								if(jnt->_forceout & FORCEOUT_motor2AngleRate){
									jnt->motor2AngleRate = dJointGetAMotorAngleRate( jnt->_joint, 1 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor2AngleRate));
								}
								if(jnt->_forceout & FORCEOUT_motor3Angle){
									jnt->motor3Angle = dJointGetAMotorAngle( jnt->_joint, 2 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor3Angle));
								}
								if(jnt->_forceout & FORCEOUT_motor3AngleRate){
									jnt->motor3AngleRate = dJointGetAMotorAngleRate( jnt->_joint, 2 );
									MARK_EVENT(X3D_NODE(jnt),offsetof(struct X3D_MotorJoint,motor3AngleRate));
								}
							}

						}
						break;
					default:
						break;
				} //switch on joint
			} //for joints
			//do collisionSensor
				//not implemented yet
			// remove all contact joints
			dJointGroupEmpty (contactgroup);
		}
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
		struct X3D_RigidBodyCollection *node = (struct X3D_RigidBodyCollection*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;

		if(!world){
			world = dWorldCreate();
			space = dHashSpaceCreate (0); //default space

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
		node->_world = (void*)world;
		node->_space = (void*)space;
		dJointGroupID groupID = dJointGroupCreate (0); //this is a contact group for joints
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
	if(_node){
		//CollidableOffset will come in here too, so in perl, keep the first fields in same order 
		//so we can caste to one, and the fields jive
		struct X3D_CollidableShape *node = (struct X3D_CollidableShape*)_node;
		if(!renderstate()->render_vp) {

			COMPILE_IF_REQUIRED

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
		if(node->collidable) child_CollidableShape(node->collidable);
	}
}

//void do_CollisionSensorTick(void * ptr){
//	struct X3D_CollisionSensor *node = (struct X3D_CollisionSensor *)ptr;
//	if(!node)return;
//}
void add_physics(struct X3D_Node *node){
	switch(node->_nodeType){
		case NODE_CollisionSensor:
			//Nov. 2016: H: this should be in do_first ie do_CollisionSensor
			//which gets called before routing ie so if there's a Contact it can be routed
			//its almost the same place as physics, which is just after do_events,routing
			// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/concepts.html#ExecutionModel
			register_CollisionSensor(node);
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

#endif	
	
	
	
	
