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
RigidBody				A,D		CollidableShape, CollidableOffset
CollisionCollection		A,D		CollidableShape, CollidableOffset, CollisionSpace
CollisionSpace			A,D		CollidableShape, CollidableOffset, CollisionSpace
CollidableShape			B		Shape
CollidableOffset		B		CollidableShape
CollisionSensor			A,D		CollisionCollection

Generated Node			Generating Node
CollisionCollection		RigidBodyCollection
Contact					CollisionSensor


*/

//some crap copied & pasted by dug9 from ode's demo_boxstack.cpp demo program:
// some constants

#define NUM 100			// max number of objects
#define DENSITY (5.0)		// density of all objects
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
static dWorldID world;
static dSpaceID space;
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



static struct Vector *x3dworlds = NULL;
static struct Vector *x3dcollisionsensors = NULL;



// this is called by dSpaceCollide when two objects in space are
// potentially colliding.

static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
  int i, numc;
  // if (o1->body && o2->body) return;

  // exit without doing anything if the two bodies are connected by a joint
  dBodyID b1 = dGeomGetBody(o1);
  dBodyID b2 = dGeomGetBody(o2);
  if (b1 && b2 && dAreConnectedExcluding (b1,b2,dJointTypeContact)) return;

  dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per box-box
  for (i=0; i<MAX_CONTACTS; i++) {
    contact[i].surface.mode = dContactBounce | dContactSoftCFM;
    contact[i].surface.mu = dInfinity;
    contact[i].surface.mu2 = 0;
    contact[i].surface.bounce = 0.1;
    contact[i].surface.bounce_vel = 0.1;
    contact[i].surface.soft_cfm = 0.01;
  }
  if (numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom,
			   sizeof(dContact))) {
    dMatrix3 RI;
    dRSetIdentity (RI);
    const dReal ss[3] = {0.02,0.02,0.02};
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

static int init_rbp_once = 0;
static dThreadingImplementationID threading;
static dThreadingThreadPoolID pool;
int init_rbp(){
	if(!init_rbp_once && world && space && contactgroup){
		init_rbp_once = TRUE;
		//c++: 
		dInitODE2(0);
		memset (obj,0,sizeof(obj));

		dThreadingImplementationID threading = dThreadingAllocateMultiThreadedImplementation();
		dThreadingThreadPoolID pool = dThreadingAllocateThreadPool(4, 0, dAllocateFlagBasicData, NULL);
		dThreadingThreadPoolServeMultiThreadedImplementation(pool, threading);
		// dWorldSetStepIslandsProcessingMaxThreadCount(world, 1);
		dWorldSetStepThreadingImplementation(world, dThreadingImplementationGetFunctions(threading), threading);

		dAllocateODEDataForThread(dAllocateMaskAll);

	}
	return init_rbp_once;
}
void finish_rbp(){
	if(init_rbp_once){

		dThreadingImplementationShutdownProcessing(threading);
		dThreadingFreeThreadPool(pool);
		dWorldSetStepThreadingImplementation(world, NULL, NULL);
		dThreadingFreeImplementation(threading);


		dJointGroupDestroy (contactgroup);
		dSpaceDestroy (space);
		dWorldDestroy (world);
		dCloseODE();
		init_rbp_once = 0;
	}

}
static int pause = FALSE;
void rbp_run_physics(){
	ppComponent_RigidBodyPhysics p;
	p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	if(init_rbp()){
		int i,j,k;
		struct X3D_RigidBodyCollection *x3dworld;
		for(j=0;j<vectorSize(x3dworlds);j++){
			struct X3D_RigidBody *x3dbody;
			struct X3D_CollidableOffset *x3doffset;
			struct X3D_CollidableShape *x3dcshape;
			x3dworld = (struct X3D_RigidBodyCollection*)vector_get(struct X3D_Node*,x3dworlds,j);
			//Collidable -> rigidbody 
			for(i=0;i<x3dworld->bodies.n;i++){
				x3dbody = (struct X3D_RigidBody*)x3dworld->bodies.p[i];
				if(!x3dbody->_body){
					x3dbody->_body = dBodyCreate (x3dworld->_world);
					dBodySetData (x3dbody->_body,(void*) x3dbody);
				}
				x3dcshape = NULL;
				x3doffset = NULL;
				for(k=0;k<x3dbody->geometry.n;k++){
					if(x3dbody->geometry.p[k]->_nodeType == NODE_CollidableOffset){
						x3doffset = (struct X3D_CollidableOffset*)x3dbody->geometry.p[k];
						x3dcshape = (struct X3D_CollidableShape*)x3doffset->collidable;
					}else if(x3dbody->geometry.p[k]->_nodeType == NODE_CollidableShape){
						x3dcshape = (struct X3D_CollidableShape*)x3dbody->geometry.p[k];
						x3doffset = (struct X3D_CollidableOffset*)x3dcshape;
					}
					if(!x3dcshape->_geom){
						//x3dcshape->_geom = 
					}
					struct SFVec3f translation = x3doffset->translation;
					struct SFRotation rotation = x3doffset->rotation;
					dBodySetPosition (x3dbody->_body, translation.c[0],translation.c[1],translation.c[2]);
					if(1){
						dReal dquat[4];
						Quaternion quat;
						vrmlrot_to_quaternion(&quat,rotation.c[0],rotation.c[1],rotation.c[2],rotation.c[3]);
						dquat[0] = quat.x; dquat[1] = quat.y, dquat[2] = quat.z, dquat[3] = quat.w;
						dBodySetQuaternion(x3dbody->_body,dquat);
					}else{
						dMatrix3 R;
						dRFromAxisAndAngle (R,rotation.c[0],rotation.c[1],rotation.c[2],rotation.c[3]);
						dBodySetRotation(x3dbody->_body,R);
					}

				}
				//dCreatePlane (space,0,0,1,0);
				//memset (obj,0,sizeof(obj));
			}
			dSpaceCollide (space,0,&nearCallback);
			if (!pause) dWorldQuickStep (world,0.02);

			//Rigidbody -> Collidable

			// remove all contact joints
			dJointGroupEmpty (contactgroup);
		}
	}

}


//void register_CollisionCollection(struct X3D_Node *node){
//	if(node->_nodeType == NODE_CollisionCollection){
//		ppComponent_RigidBodyPhysics p;
//		struct X3D_CollisionCollection *self = (struct X3D_CollisionCollection*)node;
//		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
//
//		contactgroup = dJointGroupCreate (0);
//		x3dcontact = node;
//		MARK_NODE_COMPILED;
//	}
//}
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
//void register_CollisionSpace(struct X3D_Node *node){
//	if(node->_nodeType == NODE_CollisionSpace){
//		ppComponent_RigidBodyPhysics p;
//		struct X3D_CollisionSpace *self = (struct X3D_CollisionSpace*)node;
//		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
//
//		space = dHashSpaceCreate (0);
//
//		x3dspace = node;
//
//		//MARK_NODE_COMPILED;
//	}
//}
void compile_Contact(struct X3D_Node *node){
}
//void register_RigidBody(struct X3D_Node *node){
//	if(node->_nodeType == NODE_RigidBody){
//		ppComponent_RigidBodyPhysics p;
//		struct X3D_RigidBody *self = (struct X3D_RigidBody*)node;
//		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
//
//		//MARK_NODE_COMPILED;
//	}
//}
void register_RigidBodyCollection(struct X3D_Node *_node){
	if(_node->_nodeType == NODE_RigidBodyCollection){
		ppComponent_RigidBodyPhysics p;
		struct X3D_RigidBodyCollection *node = (struct X3D_RigidBodyCollection*)_node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;

		world = dWorldCreate();
		space = dHashSpaceCreate (0); //default space

		dWorldSetGravity (world,0,0,-GRAVITY);
		dWorldSetCFM (world,1e-5);
		dWorldSetAutoDisableFlag (world,1);
		space = dHashSpaceCreate (0);
		contactgroup = dJointGroupCreate (0);

		#if 1

		dWorldSetAutoDisableAverageSamplesCount( world, 10 );

		#endif

		dWorldSetLinearDamping(world, 0.00001);
		dWorldSetAngularDamping(world, 0.005);
		dWorldSetMaxAngularSpeed(world, 200);

		dWorldSetContactMaxCorrectingVel (world,0.1);
		dWorldSetContactSurfaceLayer (world,0.001);

		node->_world = (void*)world;
		node->_space = (void*)space;
		if(!x3dworlds) x3dworlds = newVector(struct X3D_Node*,5);
		vector_pushBack(struct X3D_Node*,x3dworlds,_node);
		MARK_NODE_COMPILED;
	}
}


void prep_CollidableShape(struct X3D_Node *_node){
	if(_node){
		//CollidableOffset will come in here too, so in perl, keep the first fields in same order so we can caste to one, and the fields jive
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


void add_physics(struct X3D_Node *node){
	switch(node->_nodeType){
		case NODE_CollisionSensor:
			register_CollisionSensor(node);
			break;
		//case NODE_CollisionSpace:
		//	register_CollisionSpace(node);
		//	break;
		//case NODE_RigidBody:
		//	register_RigidBody(node);
		//	break;
		case NODE_RigidBodyCollection:
			register_RigidBodyCollection(node);
			break;
		default:
			break;
	}
}


#else //else no ode phyiscs engine, just stubs


void rbp_run_physics(){
}
void add_physics(struct X3D_Node *node){
}

//void compile_CollidableOffset(struct X3D_Node *node){
//}
//void compile_CollidableShape(struct X3D_Node *node){
//}
//void compile_CollisionCollection(struct X3D_Node *node){
//}
//void compile_CollisionSensor(struct X3D_Node *node){
//}
//void compile_CollisionSpace(struct X3D_Node *node){
//}
//void compile_Contact(struct X3D_Node *node){
//}
//void compile_RigidBody(struct X3D_Node *node){
//}
//void compile_RigidBodyCollection(struct X3D_Node *node){
//}

void prep_CollidableShape(struct X3D_Node *node){
}
void fin_CollidableShape(struct X3D_Node *node){
}
void child_CollidableShape(struct X3D_Node *node){
}
void prep_CollidableOffset(struct X3D_Node *node){
}
void fin_CollidableOffset(struct X3D_Node *node){
}
void child_CollidableOffset(struct X3D_Node *node){
}
#endif	
	
	
	
	
