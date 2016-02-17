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
void rbp_add_collisionSpace(struct X3D_Node* node);
void rbp_add_collidableShape(struct X3D_Node* node);
void rbp_add_rigidbody(struct X3D_Node* node);

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


*/


void rbp_run_physics(){
	ppComponent_RigidBodyPhysics p;
	p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;

}

void compile_CollidableOffset(struct X3D_Node *node){
}
void compile_CollidableShape(struct X3D_Node *node){
	if(node->_nodeType == NODE_CollidableShape){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollidableShape *self = (struct X3D_CollidableShape*)node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	}
}
void compile_CollisionCollection(struct X3D_Node *node){
	if(node->_nodeType == NODE_CollisionCollection){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollisionCollection *self = (struct X3D_CollisionCollection*)node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	}
}
void compile_CollisionSensor(struct X3D_Node *node){
	if(node->_nodeType == NODE_CollisionSensor){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollisionSensor *self = (struct X3D_CollisionSensor*)node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	}
}
void compile_CollisionSpace(struct X3D_Node *node){
	if(node->_nodeType == NODE_CollisionSpace){
		ppComponent_RigidBodyPhysics p;
		struct X3D_CollisionSpace *self = (struct X3D_CollisionSpace*)node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	}
}
void compile_Contact(struct X3D_Node *node){
}
void compile_RigidBody(struct X3D_Node *node){
	if(node->_nodeType == NODE_RigidBody){
		ppComponent_RigidBodyPhysics p;
		struct X3D_RigidBody *self = (struct X3D_RigidBody*)node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	}
}
void compile_RigidBodyCollection(struct X3D_Node *node){
	if(node->_nodeType == NODE_RigidBodyCollection){
		ppComponent_RigidBodyPhysics p;
		struct X3D_RigidBodyCollection *self = (struct X3D_RigidBodyCollection*)node;
		p = (ppComponent_RigidBodyPhysics)gglobal()->Component_RigidBodyPhysics.prv;
	}
}


#else //else no ode phyiscs engine, just stubs


void rbp_run_physics(){
	run_physics();
	return;
}
void compile_CollidableOffset(struct X3D_Node *node){
}
void compile_CollidableShape(struct X3D_Node *node){
}
void compile_CollisionCollection(struct X3D_Node *node){
}
void compile_CollisionSensor(struct X3D_Node *node){
}
void compile_CollisionSpace(struct X3D_Node *node){
}
void compile_Contact(struct X3D_Node *node){
}
void compile_RigidBody(struct X3D_Node *node){
}
void compile_RigidBodyCollection(struct X3D_Node *node){
}

#endif	
	
	
	
	
