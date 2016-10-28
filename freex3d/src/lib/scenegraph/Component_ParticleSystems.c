/*


X3D Particle Systems Component

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
#include "RenderFuncs.h"
#include "LinearAlgebra.h"
//#include "Component_ParticleSystems.h"
#include "Children.h"
#include "Component_Shape.h"
#include "../opengl/Textures.h"

typedef struct pComponent_ParticleSystems{
	int something;
}* ppComponent_ParticleSystems;
void *Component_ParticleSystems_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_ParticleSystems));
	memset(v,0,sizeof(struct pComponent_ParticleSystems));
	return v;
}
void Component_ParticleSystems_init(struct tComponent_ParticleSystems *t){
	//public
	//private
	t->prv = Component_ParticleSystems_constructor();
	{
		ppComponent_ParticleSystems p = (ppComponent_ParticleSystems)t->prv;
		p->something = 0;
	}
}
void Component_ParticleSystems_clear(struct tComponent_ParticleSystems *t){
	//public
}

//ppComponent_ParticleSystems p = (ppComponent_ParticleSystems)gglobal()->Component_ParticleSystems.prv;

/*	Particle Systems
	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html
	examples:
		Non- x3d:
			https://stemkoski.github.io/Three.js/#particlesystem-shader
		x3d scenes:
	links:
		http://mmaklin.com/uppfra_preprint.pdf
		Nice particle physics
		http://www.nvidia.com/object/doc_characters.html
		Nvidia link page for game programmming with shaders

	Fuzzy Design:
	1. Update position of particles from a particleSystem node
		Eval particles after events (after the do_tick) and befre RBP physics
			see http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/concepts.html#ExecutionModel
		positions are update wrt the local system of the particleSystme node
		each particle has a struct { lifetime remaining, position, velocity vector, ??}
		up to 10,000 particles (per particle node)
		randomizing: use C srand(time) once, and rand() for each randomizing. Scale by Variation field
		CPU design: iterate over particles, updating each one
		GPU design ie openCL: do same thing in massive parallel
	2. Render
		during geom pass of render_hier, in render_particleSystem()
		CPU design: iterate over particles like children:
			updating transform stack with particle position
			updating appearance f(time)
			calling render_node(node) on each particle
		GPU design: send arrray of particle positions/states to GPU
			in shader iterate over positions, re-rendering for each
			
	PROBLEM with trying to do physics in shader: 
	x how do you update the state of each particle in a way the next frame can access?
	vs on cpu, if you have 80 particles, you can whip through them, updating their state, 
	- and resending the state via attribute array on each frame
	- also more flexible when doing geometryType="GOEMETRY" / goemetry node, that code is CPU

	PROBLEM with sending just the particle position, and generating the sprite geometry
	in the shader: GLES2 doesn't have gometry shaders.

	GLES2 has no gl_VertexID per vertex in vertex shader, so:
	- send glAttributeArray of xyz positions to match vertices?
	- send repetitive triangles - same 3 xyz repetitively?
	- in shader add position to triangle verts?
	- or send glAttributeArray of sprite ID of length nvert
	-- and uniform3fv of xyz of length nsprite
	-- then lookup xyz[spriteID] in vertex shader?

	EASIEST CPU/GPU SPLIT:
		1. just send geometry for 1 particle to shader
		2. cpu loop over particles:
			foreach liveparticle
				send position to shader
				send texcoord to shader
				send cpv to shader
				gl_DrawArrays

	POSITION
	because position is just xyz (not orientation or scale) the shader could
		take a vec3 for that, and add it on before transforming from local to view
	for non-GEOMETRY, the transform needs to keep the face normal parallel to the view Z
		H: you could do that by transforming 0,0,0 to view, and adding on gl_vertex 
		x but that wouldn't do scale, or orientation if you need it
		- for scale also transform gl_vertex, get the |diff| from 0 for scale

	PHYSICS - I don't see any rotational momentum needs, which involve cross products
	- so forces F, positions p, velocities v, accelerations a are vec3
	- mass, time are scalars
	- physics:
	  F = m * a
	   a = F/m
	  v2 = v1 + a*dt
	  p2 = p1 + .5(v1 + v2)*dt
	  p2 = p1 + v1*dt + .5*a*dt**2 
	   p - position
	   v - velocity
	   a - acceleration
	   dt - delta time = (time2 - time1)
	   m - mass
	   F - force

	RANDOM DIRECTIONS
	http://math.stackexchange.com/questions/44689/how-to-find-a-random-axis-or-unit-vector-in-3d

	RANDOM TRIANGLE COORDS
	picking a random triangle won't evenly distribute by area, so we can be approx with point inside tri too
	can pick 2 0-1 range numbers, and use as barycentric coords b1,b2:
	https://en.wikipedia.org/wiki/Barycentric_coordinate_system
	p = b1*p1 + b2*p2 + (1 - b1 - b2)*p3

	COMPARISONS - H3D, Octaga, Xj3D claim particle physics
	- Octaga responds to particle size, has good force and wind effects

Its like a Shape node, or is a shape node, 
set geometry
set basic appearance
foreach liveparticle
	update texcoord
	update color (color per vertex)
	update position
	gl_DrawArrays or gl_DrawElements

Options in freewrl:
1. per-particle child_Shape 
	foreach liveparticle
		push particle position onto transform stack
		fiddle with appearance node on child
		call child_Shape(particle)
2. per-particle-system child_Shape
	call child_Shape
		if(geometryType 'GEOMETRY')
		.... sendArraysToGPU (hack)
				foreach liveparticle
					update position
					glDrawArrays
		.... sendElementsToGPU (hack)
				foreach liveparticle
					update position
					glDrawElements
		if(geometryType !GEOMETRY)
			send vbo with one line or quad or 2 triangles or point
			foreach liveparticle
				update position
				update texcoord
				update color (color per vertex)
				gl_DrawArrays or gl_DrawElements
3. refactor child shape to flatten the call hierarchy
	child shape:
	a) determine shader flags
	b) compile/set/bind shader program
	c) set appearance - pull setupShader out of sendArraysToGPU
		set material
		set texture
		set shader
		...
	d) set geometry vertices and type element/array, line/triangle
		if GEOMETRY: render(geom node) - except don't call gl_DrawArrays or gl_DrawElements
		PROBLEM: some geometries -cylinder, cone- are made from multiple calls to gl_DrawElements
		OPTION: refactor so uses polyrep, or single permuatation call
				(cylinder: 4 permutations: full, bottom, top, no-ends)
		else !GEOMETRY: send one quad/line/point/triangle 
	e) foreach liveparticle
		update position
		update texcoord
		update color (CPV)
		gl_Draw xxx: what was set in d) above
4. half-flatten child_shape
	as in #3, except just take setupShader out of sendArraysToGPU/sendElementsToGPU, and put in 
		child_shape body
	then child_particlesystem can be a copy of child_shape, with loop over particles 
		calling render_node(geometry) for GEOMETRY type (resending vbos)
5. make sendArrays etc a callback function, different for particles
CHOICE: #3 
	setup shader //sends materials, matrices to shader
	render_node(geometry) //sends vertex data to shader, saves call parameters to gl_DrawArrays/Elements
	foreach liveparticle
		update particle-specific position, color, texcoords
		reallyDrawOnce() //calls glDrawArrays or Elements
	clearDraw()

*/


float uniformRand(){
	// 0 to 1 inclusive
	static int once = 0;
	unsigned int ix;
	float rx;

	if(!once)
		srand((unsigned int) TickTime());
	once = 1;
	ix = rand();  
	rx = (float)ix/(float)RAND_MAX; //you would do (RAND_MAX - 1) here for excluding 1
	return rx;
}
float uniformRandCentered(){
	return uniformRand() - .5f; //center on 0
}
void circleRand2D(float *xy){
	//random xy on a circle area radius 1
	float radius2;
	for(;;){
		xy[0] = 2.0f*(uniformRand() - .5f);
		xy[1] = 2.0f*(uniformRand() - .5f);
		radius2 = xy[0]*xy[0] + xy[1]*xy[1];
		if(radius2 <= 1.0f) break;
	}
}
float normalRand(){
	// in -.5 to .5 range
	float rxy[2];
	// by just taking points in a circle radius, this emulates the falloff of a normal curve in one dimension
	//     .
	//  . x    .
	// :        :
	//  .      .
	//     .   o
	//
	circleRand2D(rxy);
	return (rxy[0]*.5); //scale from -1 to 1 into -.5 to .5 range
}

void randomTriangleCoord_dug9_uneducated_guess(float *p, float* p1, float *p2, float *p3){
	// get 2 random barycentric coords 0-1, and use those
	// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
	// x I think b1 + b2 can be > 1.0 and thats wrong
	int i;
	float b1, b2;
	b1 = uniformRand();
	b2 = uniformRand();
	for(i=0;i<3;i++){
		p[i] = b1*p1[i] + b2*p2[i] + (1.0f - b1 - b2)*p3[i];
	}
}
void randomTriangleCoord(float *p, float* p1, float *p2, float *p3){
	// http://math.stackexchange.com/questions/18686/uniform-random-point-in-triangle
	int i;
	float r1, r2, sqr1,sqr2;
	r1 = uniformRand();
	r2 = uniformRand();
	sqr1 = sqrtf(r1);
	sqr2 = sqrtf(r2);
	for(i=0;i<3;i++){
		p[i] = (1.0f - sqr1)*p1[i] + (sqr1*(1.0f - sqr2))*p2[i] + (r2*sqr1)*p3[i];
	}

}
void randomDirection(float *xyz){
	//random xyz direction from a point
	//http://math.stackexchange.com/questions/44689/how-to-find-a-random-axis-or-unit-vector-in-3d
	float radius3;
	for(;;){
		//get random point in a unit cube
		xyz[0] = (uniformRand() - .5f);
		xyz[1] = (uniformRand() - .5f);
		xyz[2] = (uniformRand() - .5f);
		//discard point if outside unit sphere
		radius3 = xyz[0]*xyz[0] + xyz[1]*xyz[1] + xyz[2]*xyz[2];
		if(radius3 <= 1.0f && radius3 > 0.0000001f){
			//vecnormalize3f(xyz,xyz);
			//normalize direction to point
			vecscale3f(xyz,xyz,1.0f/sqrtf(radius3));
			break;
		}
	}
}

typedef struct {
	//store at end of current iteration, for use on next iteration
	float age;
	float lifespan; //assigned at birth
	float size[2];  //assigned at birth
	float position[3];
	float velocity[3];
	float origin[3]; //zero normally. For boundedphysics, updated on each reflection to be last reflection point.
	//float direction[3];
	//float speed;
	float mass;
	float surfaceArea;
} particle;
enum {
	GEOM_QUAD = 1,
	GEOM_LINE = 2,
	GEOM_POINT = 3,
	GEOM_SPRITE = 4,
	GEOM_TRIANGLE = 5,
	GEOM_GEOMETRY = 6,
};
struct {
const char *name;
int type;
} geomtype_table [] = {
{"QUAD",GEOM_QUAD},
{"LINE",GEOM_LINE},
{"POINT",GEOM_POINT},
{"SPRITE",GEOM_SPRITE},
{"TRIANGLE",GEOM_TRIANGLE},
{"GEOMETRY",GEOM_GEOMETRY},
{NULL,0},
};
int lookup_geomtype(const char *name){
	int iret,i;
	iret=i=0;
	for(;;){
		if(geomtype_table[i].name == NULL) break;
		if(!strcmp(geomtype_table[i].name,name)){
			iret = geomtype_table[i].type;
			break;
		}
		i++;
	}
	return iret;
}
//GLfloat quadtris [18] = {1.0f,1.0f,0.0f, -1.0f,1.0f,0.0f, -1.0f,-1.0f,0.0f,    1.0f,1.0f,0.0f, -1.0f,-1.0f,0.0f, 1.0f,-1.0f,0.0f};
GLfloat quadtris [18] = {-.5f,-.5f,0.0f, .5f,-.5f,0.0f, .5f,.5f,0.0f,   .5f,.5f,0.0f, -.5f,.5f,0.0f, -.5f,-.5f,0.0f,};
GLfloat twotrisnorms [18] = {0.f,0.f,1.f, 0.f,0.f,1.f, 0.f,0.f,1.f,    0.f,0.f,1.f, 0.f,0.f,1.f, 0.f,0.f,1.f,};
GLfloat twotristex [12] = {0.f,0.f, 1.f,0.f, 1.f,1.f,    1.f,1.f, 0.f,1.f, 0.f,0.f};


// COMPILE PARTICLE SYSTEM
void compile_ParticleSystem(struct X3D_ParticleSystem *node){
	int i,j, maxparticles;
	float *boxtris, *vertices;
	Stack *_particles;

	ConsoleMessage("compile_particlesystem\n");
	//delegate to compile_shape - same order to appearance, geometry fields
	compile_Shape((struct X3D_Shape*)node);

	node->_geometryType = lookup_geomtype(node->geometryType->strptr);
	if(node->_tris == NULL){
		node->_tris = MALLOC(void *,18 * sizeof(float));
		//memcpy(node->_tris,quadtris,18*sizeof(float));
	}
	vertices = (float*)(node->_tris);
	//rescale vertices, in case scale changed
	for(i=0;i<6;i++){
		float *vert, *vert0;
		vert0 = &quadtris[i*3];
		vert = &vertices[i*3];
		vert[0] = vert0[0]*node->particleSize.c[0];
		vert[1] = vert0[1]*node->particleSize.c[1];
		vert[2] = vert0[2];
	}
	if(node->texCoordRamp){
		int ml,mq,mt,n;
		struct X3D_TextureCoordinate *tc = (struct X3D_TextureCoordinate *)node->texCoordRamp;
		n = node->texCoordKey.n;
		mq = n*4; //quad
		ml = n*2; //2 pt line
		mt = n*6; //2 triangles

		//malloc for both lines and tex, in case changed on the fly
		if(!node->_ttex)
			node->_ttex = MALLOC(void *,mt*2*sizeof(float));
		if(!node->_ltex)
			node->_ltex = MALLOC(void *,ml*2*sizeof(float));
		if(tc->point.n == mq){
			//enough tex coords for quads, expand to suit triangles
			//  4 - 3
			//  5 / 2  2 triangle config
			//  0 _ 1
			float *ttex, *ltex;
			ttex = (float*)node->_ttex;
			for(i=0;i<n;i++){
				int k;
				for(j=0,k=0;j<4;j++,k++){
					float *p = (float*)(float *)&tc->point.p[i*4 + j];
					veccopy2f(&ttex[(i*6 + k)*2],p);
					if(k==0){
						veccopy2f(&ttex[(i*6 + 5)*2],p); //copy to 5 (last of 0-6 2-triangle)
					}
					if(k==2){
						k++;
						veccopy2f(&ttex[(i*6 + k)*2],p); //copy 2 to 3 (start of 2nd triangle
					}
				}
			}
			if(0) for(i=0;i<n;i++){
				for(j=0;j<6;j++)
					printf("%f %f,",ttex[(i*6 + j)*2 +0],ttex[(i*6 + j)*2 +1]);
				printf("\n");
			}
			//for(i=0;i<(n*6*2);i++){
			//	printf("%f \n",ttex[i]);
			//}

			ltex = (float*)node->_ltex;
			for(i=0;i<n;i++){
				// make something up for lines
				for(j=0;j<2;j++){
					float p[2];
					struct SFVec2f *sf = (struct SFVec2f *)&tc->point.p[i*4 + j];
					p[0] = sf->c[0];
					p[1] = min(sf->c[1],.9999); //clamp texture here otherwise tends to wrap around
					veccopy2f(&ltex[(i*2 + j)*2],p);
					
				}
			}
		}
		if(tc->point.n == ml){
			//enough points for lines
			float *ttex, *ltex;

			ltex = (float*)node->_ltex;
			for(i=0;i<n;i++){
				// copy lines straightforwardly
				for(j=0;j<2;j++){
					float p[2];
					struct SFVec2f *sf = (struct SFVec2f *)&tc->point.p[i*2 + j];
					p[0] = sf->c[0];
					p[1] = min(sf->c[1],.9999); //clamp texture here otherwise tends to wrap around
					veccopy2f(&ltex[(i*2 + j)*2],p);
				}
			}
			if(0) for(i=0;i<n;i++){
				printf("%f %f, %f %f\n",ltex[i*2*2 + 0],ltex[i*2*2 + 1],ltex[i*2*2 + 2],ltex[i*2*2 + 3]);
			}
			//make something up for triangles
			ttex = (float*)node->_ttex;
			for(i=0;i<n;i++){
				float *p;
				j = i;
				p = (float*)(float *)&tc->point.p[j*2 + 0];
				veccopy2f(&ttex[(i*6 + 0)*2],p); //copy to 0 
				veccopy2f(&ttex[(i*6 + 5)*2],p); //copy to 5
				p = (float*)(float *)&tc->point.p[j*2 + 1];
				veccopy2f(&ttex[(i*6 + 1)*2],p); //copy to 1
				j++;
				j = j == n ? j - 1 : j; //clamp to last
				p = (float*)(float *)&tc->point.p[j*2 + 1];
				veccopy2f(&ttex[(i*6 + 2)*2],p); //copy to 2
				veccopy2f(&ttex[(i*6 + 3)*2],p); //copy to 3
				p = (float*)(float *)&tc->point.p[j*2 + 0];
				veccopy2f(&ttex[(i*6 + 4)*2],p); //copy to 4
			}
		}
	}
	maxparticles = min(node->maxParticles,10000);
	if(node->_particles == NULL)
		node->_particles = newVector(particle,maxparticles);
	_particles = node->_particles;
	if(_particles->allocn < maxparticles) {
		//resize /realloc vector, set nalloc, in case someone changed maxparticles on the fly
		_particles->data = realloc(_particles->data,maxparticles);
		_particles->allocn = maxparticles;
	}
	node->_lasttime = TickTime();
	if(node->enabled){
		node->isActive = TRUE;
		MARK_EVENT (X3D_NODE(node),offsetof (struct X3D_ParticleSystem, isActive));
	}
	MARK_NODE_COMPILED
}

//PHYSICS
void prep_windphysics(struct X3D_Node *physics){
	//per-frame gustiness update
	struct X3D_WindPhysicsModel *px = (struct X3D_WindPhysicsModel *)physics;
	float speed;
	speed = px->speed * (1.0f + uniformRandCentered()*px->gustiness);
	px->_frameSpeed = speed;
}
void apply_windphysics(particle *pp, struct X3D_Node *physics, float dtime){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html#WindPhysicsModel
	struct X3D_WindPhysicsModel *px = (struct X3D_WindPhysicsModel *)physics;
	if(px->enabled && pp->mass != 0.0f){
		float pressure;
		float force, speed;
		float turbdir[3], pdir[3];
		speed = px->_frameSpeed;
		pressure = powf(10.0f,2.0f*log10f(speed)) * .64615f;
		force = pressure * pp->surfaceArea;
		randomDirection(turbdir);
		vecscale3f(turbdir,turbdir,px->turbulence);
		vecadd3f(pdir,px->direction.c,turbdir);
		vecnormalize3f(pdir,pdir);
		vecscale3f(pdir,pdir,force);

		float acceleration[3], v2[3];
		vecscale3f(acceleration,pdir,1.0f/pp->mass);
		vecscale3f(v2,acceleration,dtime);
		vecadd3f(pp->velocity,pp->velocity,v2);

	}
}
int intersect_polyrep(struct X3D_Node *node, float *p1, float *p2, float *nearest, float *normal);

void compile_geometry(struct X3D_Node *gnode){
	//this needs generalizing, for all triangle geometry, and I don't know how
	if(gnode)
	switch(gnode->_nodeType){
		case NODE_IndexedFaceSet:
		{
			struct X3D_IndexedFaceSet *node = (struct X3D_IndexedFaceSet *)gnode;
			COMPILE_POLY_IF_REQUIRED (node->coord, node->fogCoord, node->color, node->normal, node->texCoord)
		}
		break;
		default:
		break;
	}
	
}
int intersect_geometry(struct X3D_Node *gnode, float *p1, float *p2, float *nearest, float *normal){
	//this needs generalizing for all triangle geometry 
	int iret = 0;
	if(gnode)
	switch(gnode->_nodeType){
		case NODE_IndexedFaceSet:
			iret = intersect_polyrep(gnode,p1,p2,nearest,normal);
		break;
		default:
		break;
	}
	return iret;
}
void apply_boundedphysics(particle *pp, struct X3D_Node *physics, float *positionChange){
	struct X3D_BoundedPhysicsModel *px = (struct X3D_BoundedPhysicsModel *)physics;
	if(px->enabled && px->geometry ) {   //&& pp->mass != 0.0f){
		//shall we assume its a 100% elastic bounce?
		int nintersections, ntries;
		struct X3D_Node *node = (struct X3D_Node *) px->geometry;
		float pos1[3], pos2[3], pnearest[3],normal[3], delta[3];
		//make_IndexedFaceSet((struct X3D_IndexedFaceSet*)px->geometry);
		//COMPILE_POLY_IF_REQUIRED (node->coord, node->fogCoord, node->color, node->normal, node->texCoord)
		if(NODE_NEEDS_COMPILING)
			compile_geometry(node);

		//if polyrep
		//veccopy3f(pos1,pp->position);

		veccopy3f(pos1,pp->origin);
		//vecadd3f(pos2,pp->position,positionChange);
		vecadd3f(pos2,pp->position,positionChange);
		ntries = 0;
		static int count = 0;
		//for(;;) //in theory we may travel far enough for 2 bounces
		{
			//if(pos2[0] >= .5f)
			//	printf("should hit\n");
			nintersections = intersect_geometry(px->geometry,pos1,pos2,pnearest,normal);
			if(nintersections > 0){
				count++;
				float d[3], r[3], rn[3], rd[3], n[3], ddotnn[3], orthogn[3], orthogn2[3];
				float ddotn, speed, dlengthi,dlength;
				vecdif3f(delta,pos2,pos1);
				dlength = veclength3f(delta);

				// get reflection 
				//  pos1
				// o|\d
				// n<--x
				// o|/r
				// ddotn = dot(d,n)*n //projection of d onto n, as vector
				// o = d - ddotn  //vector orthogonal to n, such that d = ddotn + o
				// r = ddotn - o 
				// or 
				// r = ddotn - (d - ddotn)
				// or
				// r = 2*ddotn - d
				// or
				// r = -(d - 2*dot(d,n)*n)
				// http://math.stackexchange.com/questions/13261/how-to-get-a-reflection-vector
			
				vecdif3f(d,pnearest,pos1);
				dlengthi = veclength3f(d);
				vecnormalize3f(n,normal);
				ddotn = vecdot3f(d,n);
				//ddotn = -ddotn; //assuming the surface normal is pointing out
				vecscale3f(ddotnn,n,ddotn);
				vecdif3f(orthogn,d,ddotnn);
				vecscale3f(orthogn2,orthogn,2.0f);
				vecdif3f(r,d,orthogn2);
				vecscale3f(r,r,-1.0f); //reverse direction
				vecnormalize3f(rn,r);
				// update the velocity vector direction (keep speed constant, assuming elastic bounce)
				// specs: could use an elasticity factor
				speed = veclength3f(pp->velocity);
				vecscale3f(pp->velocity,rn,speed);
				//do positionChange here, and zero positionchange for calling code
				vecscale3f(rd,rn,dlength - dlengthi);
				vecadd3f(pp->position,pnearest,rd);
				vecscale3f(positionChange,positionChange,0.0f);
				veccopy3f(pos1,pnearest);
				veccopy3f(pp->origin,pos1);
				veccopy3f(pos2,pp->position);
				if(0) pp->age = 1000.0f; //simply expire if / when goes outside 
				// specs could add death-on-hitting-wall
			}
			//break;
			//if(nintersections == 0)break;
			//ntries++;
			//if(ntries > 3)break;
		}
	}
}
void apply_forcephysics(particle *pp, struct X3D_Node *physics, float dtime){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html#ForcePhysicsModel
	// I think Octaga mis-interprets the [0 -9.81 0] force as acceleartion of gravity.
	// it will be if mass==1. Otherwise you need to scale your force by mass:
	// ie if your mass is 10, then force needs to be [0 98.1 0]
	struct X3D_ForcePhysicsModel *px = (struct X3D_ForcePhysicsModel *)physics;
	//a = F/m;
	//v += a*dt
	if(px->enabled && pp->mass != 0.0f){
		float acceleration[3], v2[3];
		vecscale3f(acceleration,px->force.c,1.0f/pp->mass);
		vecscale3f(v2,acceleration,dtime);
		vecadd3f(pp->velocity,pp->velocity,v2);
	}
}

//EMITTERS
void apply_ConeEmitter(particle *pp, struct X3D_Node *emitter){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html#ConeEmitter
	// like point emitter, except we need work in the direction:
	// 2 randoms, one for angle-from-direction < e.angle, and one for angle-around-direction 0-2PI
	struct X3D_ConeEmitter *e = (struct X3D_ConeEmitter *)emitter;
	float direction[3], tilt, azimuth, speed;
	{
		//prep - can be done once per direction
		//need 2 orthogonal axes 
		//a) find a minor axis
		vecnormalize3f(direction,e->direction.c);
		float amin = min(min(abs(direction[0]),abs(direction[1])),abs(direction[2]));
		int i,imin = 0;
		for(i=0;i<3;i++){
			if(abs(direction[i]) == amin){
				imin = i; break;
			}
		}
		//make a vector with the minor axis dominant
		float orthog1[3],orthog2[3];
		for(i=0;i<3;i++) orthog1[i] = 0.0f;
		orthog1[imin] = 1.0f;
		//orthog1 will only be approximately orthogonal to direction
		//do a cross product to get ortho2
		veccross3f(orthog2,direction,orthog1);
		//orthog2 will be truely orthogonal
		//cross orthog2 with direction to get truely orthog1
		veccross3f(orthog1,direction,orthog2);

		//for this particle
		int method = 2;
		if(method == 1){
			//METHOD 1: TILT + AZIMUTH
			//tends to crowd/cluster around central direction, and fade with tilt
			//(due to equal chance of tilt angle, but larger area to cover as tilt goes up)
			//direction = cos(tilt)*direction
			//az = cos(azimuth)*orthog1 + sin(azimuth)*orthog2
			//az = sin(tilt)*az
			//direction += az;
			//where
			// tilt - from e.direction axis
			// azimuth - angle around e.direction vector (in plane orthogonal to direction vector)
			// az[3] - vector in the orthogonal plane, in direction of azimuth
			float az[3],az1[3],az2[3],ctilt,stilt,caz,saz;
			tilt = uniformRand()*e->angle;
			ctilt = cosf(tilt);
			stilt = sinf(tilt);
			azimuth = uniformRand()*2.0f*PI;
			caz = cosf(azimuth);
			saz = sinf(azimuth);
			vecscale3f(az1,orthog1,caz);
			vecscale3f(az2,orthog2,saz);
			vecadd3f(az,az1,az2);
			//now az is a unit vector in orthogonal plane, in direction of azimuth
			vecscale3f(az,az,stilt);
			//now az is scaled for adding to direction
			vecscale3f(direction,direction,ctilt);
			//direction is shrunk (or reversed) to account for tilt
			vecadd3f(direction,direction,az);
			//now direction is unit vector in tilt,az direction from e.direction
		}
		if(method == 2){
			//METHOD 2: POINT IN CIRCLE
			//tends to give even distribution over circle face of cone
			//xy = randomCircle
			//orthog = x*orthog1 + y*orthog2
			//direction += orthog
			float xy[2], orx[3],ory[3], orthog[3];
			circleRand2D(xy);
			//orthog = x*orthog1 + y*orthog2
			vecscale3f(orx,orthog1,xy[0]);
			vecscale3f(ory,orthog2,xy[1]);
			vecadd3f(orthog,orx,ory);
			vecscale3f(orthog,orthog,sinf(e->angle));
			vecscale3f(direction,direction,cosf(e->angle));
			//direction += orthog
			vecadd3f(direction,orthog,direction);
			//normalize(direction)
			vecnormalize3f(direction,direction);
		}

	}
	memcpy(pp->position,e->position.c,3*sizeof(float));
	speed = e->speed*(1.0f + uniformRandCentered()*e->variation);
	vecscale3f(pp->velocity,direction,speed);
	pp->mass = e->mass*(1.0f + uniformRandCentered()*e->variation);
	pp->surfaceArea = e->surfaceArea*(1.0f + uniformRandCentered()*e->variation);

}
void apply_ExplosionEmitter(particle *pp, struct X3D_Node *emitter){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html#ExplosionEmitter
	// like point emitter, except always random direction
	// the create-all-at-time-zero is handled up one level
	struct X3D_ExplosionEmitter *e = (struct X3D_ExplosionEmitter *)emitter;
	float direction[3], speed;
	memcpy(pp->position,e->position.c,3*sizeof(float));
	randomDirection(direction);
	speed = e->speed*(1.0f + uniformRandCentered()*e->variation);
	vecscale3f(pp->velocity,direction,speed);
	pp->mass = e->mass*(1.0f + uniformRandCentered()*e->variation);
	pp->surfaceArea = e->surfaceArea*(1.0f + uniformRandCentered()*e->variation);
}
void apply_PointEmitter(particle *pp, struct X3D_Node *emitter){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html#PointEmitter
	// like explosion, except may have a non-random direction
	struct X3D_PointEmitter *e = (struct X3D_PointEmitter *)emitter;
	float direction[3], speed;
	memcpy(pp->position,e->position.c,3*sizeof(float));
	if(veclength3f(e->direction.c) < .00001){
		randomDirection(direction);
	}else{
		memcpy(direction,e->direction.c,3*sizeof(float));
		vecnormalize3f(direction,direction);
	}
	speed = e->speed*(1.0f + uniformRandCentered()*e->variation);
	vecscale3f(pp->velocity,direction,speed);
	pp->mass = e->mass*(1.0f + uniformRandCentered()*e->variation);
	pp->surfaceArea = e->surfaceArea*(1.0f + uniformRandCentered()*e->variation);
	
}
enum {
	POLYLINEEMITTER_METHODA = 1,
	POLYLINEEMITTER_METHODB = 2,
};
void compile_PolylineEmitter(struct X3D_Node *node){
	struct X3D_PolylineEmitter *e = (struct X3D_PolylineEmitter *)node;
	float *segs = NULL;
	//e->_method = POLYLINEEMITTER_METHODA;
	e->_method = POLYLINEEMITTER_METHODB;
	if(e->coord && e->coordIndex.n > 1){
		//convert IndexedLineSet to pairs of coordinates
		int i,k,ind, n,nseg = 0;
		float *pts[2];
		struct X3D_Coordinate *coord = (struct X3D_Coordinate *)e->coord;
		n = e->coordIndex.n;
		segs = MALLOC(void*,2*3*sizeof(float) *n*2 ); //2 vertices per lineseg, and 1 lineseg per coordindex should be more than enough
		k = 0;
		nseg = 0;
		for(i=0;i<e->coordIndex.n;i++){
			ind = e->coordIndex.p[i];
			if( ind == -1) {
				k = 0;
				continue;
			}
			pts[k] = (float*)&coord->point.p[ind];
			k++;
			if(k==2){
				veccopy3f(&segs[(nseg*2 +0)*3],pts[0]);
				veccopy3f(&segs[(nseg*2 +1)*3],pts[1]);
				pts[0] = pts[1];
				nseg++;
				k = 1;
			}
		}
		e->_segs = segs;
		e->_nseg = nseg;
	}
	if(e->_method == POLYLINEEMITTER_METHODB){
		int i;
		float *portions, totaldist, dist, delta[3];
		portions = MALLOC(float *,e->_nseg * sizeof(float));
		e->_portions = portions;
		totaldist = 0.0f;
		for(i=0;i<e->_nseg;i++){
			vecdif3f(delta,&segs[(i*2 + 1)*3],&segs[(i*2 + 0)*3]);
			dist = veclength3f(delta);
			//printf("dist %d %f\n",i,dist);
			portions[i] = dist;
			totaldist += dist;
		}
		for(i=0;i<e->_nseg;i++){
			portions[i] = portions[i]/totaldist;
			//printf("portion %d %f\n",i,portions[i]);
		}
	}
	MARK_NODE_COMPILED
}
void apply_PolylineEmitter(particle *pp, struct X3D_Node *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html#PolylineEmitter
	struct X3D_PolylineEmitter *e = (struct X3D_PolylineEmitter *)node;
	//like point emitter, except posiion is drawn randomly along polyline
	//option A: pick segment index at random, then pick distance along segment at random (Octaga?)
	//option B: pick random 0-1, then map that to cumulative distance along polyline
	if(NODE_NEEDS_COMPILING)
		compile_PolylineEmitter(node);
	memset(pp->position,0,3*sizeof(float)); //in case no coords/polyline/segs
	if(e->_method == POLYLINEEMITTER_METHODA && e->_nseg){
		float *segs, delta[3], pos[3];
		// pick a segment at random:
		int iseg = (int) floorf(uniformRand() * (float)e->_nseg);
		//pick a point on the segment
		float fraction = uniformRand();
		segs = (float *)e->_segs;
		vecdif3f(delta,&segs[(iseg*2 + 1)*3],&segs[(iseg*2 + 0)*3]);
		vecscale3f(delta,delta,fraction);
		vecadd3f(pos,&segs[(iseg*2 + 0)*3],delta);
		veccopy3f(pp->position,pos);
	}
	if(e->_method == POLYLINEEMITTER_METHODB && e->_nseg){
		//pick rand 0-1
		int i;
		float cumulative, fraction, *portions, *segs, delta[3], pos[3], segfraction;
		fraction = uniformRand();
		portions = (float*)e->_portions;
		cumulative = 0.0f;
		for(i=0;i<e->_nseg;i++){
			cumulative +=portions[i];
			if(cumulative > fraction){
				segfraction = (cumulative - fraction) / portions[i];
				segs = (float *)e->_segs;
				vecdif3f(delta,&segs[(i*2 + 1)*3],&segs[(i*2 + 0)*3]);
				vecscale3f(delta,delta,segfraction);
				vecadd3f(pos,&segs[(i*2 + 0)*3],delta);
				veccopy3f(pp->position,pos);
				break;
			}
		}
	}

	//the rest is like point emitter:
	float direction[3], speed;
//not for polyline see above	memcpy(pp->position,e->position.c,3*sizeof(float));
	if(veclength3f(e->direction.c) < .00001){
		randomDirection(direction);
	}else{
		memcpy(direction,e->direction.c,3*sizeof(float));
		vecnormalize3f(direction,direction);
	}
	speed = e->speed*(1.0f + uniformRandCentered()*e->variation);
	vecscale3f(pp->velocity,direction,speed);
	pp->mass = e->mass*(1.0f + uniformRandCentered()*e->variation);
	pp->surfaceArea = e->surfaceArea*(1.0f + uniformRandCentered()*e->variation);

}
void apply_SurfaceEmitter(particle *pp, struct X3D_Node *emitter){
	struct X3D_SurfaceEmitter *e = (struct X3D_SurfaceEmitter *)emitter;
}
void apply_VolumeEmitter(particle *pp, struct X3D_Node *emitter){
	struct X3D_VolumeEmitter *e = (struct X3D_VolumeEmitter *)emitter;
}
void updateColorRamp(struct X3D_ParticleSystem *node, particle *pp, GLint cramp){
	int j,k,ifloor, iceil, found;
	float rgbaf[4], rgbac[4], rgba[4], fraclife;
	found = FALSE;
	fraclife = pp->age / pp->lifespan;
	for(j=0;j<node->colorKey.n;j++){
		if(node->colorKey.p[j] <= fraclife && node->colorKey.p[j+1] > fraclife){
			ifloor = j;
			iceil = j+1;
			found = TRUE;
			break;
		}
	}
	if(found){
		float spread, fraction;
		struct SFColorRGBA * crgba = NULL;
		struct SFColor *crgb = NULL;
		switch(node->colorRamp->_nodeType){
			case NODE_ColorRGBA: crgba = ((struct X3D_ColorRGBA *)node->colorRamp)->color.p; break;
			case NODE_Color: crgb = ((struct X3D_Color *)node->colorRamp)->color.p; break;
			default:
			break;
		}
		spread = node->colorKey.p[iceil] - node->colorKey.p[ifloor];
		fraction = (fraclife - node->colorKey.p[ifloor]) / spread;
		if(crgba){
			memcpy(rgbaf,&crgba[ifloor],sizeof(struct SFColorRGBA));
			memcpy(rgbac,&crgba[iceil],sizeof(struct SFColorRGBA));
		}else if(crgb){
			memcpy(rgbaf,&crgb[ifloor],sizeof(struct SFColor));
			rgbaf[3] = 1.0f;
			memcpy(rgbac,&crgb[iceil],sizeof(struct SFColor));
			rgbac[3] = 1.0f;
		}
		for(k=0;k<4;k++){
			rgba[k] = (1.0f - fraction)*rgbaf[k] + fraction*rgbac[k];
		}
		glUniform4fv(cramp,1,rgba);
	}else{
		//re-use last color
	}
}
void updateTexCoordRamp(struct X3D_ParticleSystem *node, particle *pp, float *texcoord){
	int found, ifloor,j;
	float fraclife, fracKey;
	fraclife = pp->age / pp->lifespan;
	fracKey = 1.0f / (float)(node->texCoordKey.n); 
	//if(node->_geometryType != GEOM_LINE)
	fraclife -= fracKey; //for 3 keys, fracKey will be .333
	//    v   0000 v 1111111 v 222    change points
	//        0        .5        1  key
	for(j=0;j<node->texCoordKey.n;j++){
		if( node->texCoordKey.p[j] > fraclife){
			ifloor = j;
			found = TRUE;
			break;
		}
	}
	if(found){
		struct X3D_TextureCoordinate *tc = (struct X3D_TextureCoordinate *)node->texCoordRamp;
		switch(node->_geometryType){
			case GEOM_LINE:
				FW_GL_TEXCOORD_POINTER (2,GL_FLOAT,0,(float *)&texcoord[ifloor*2*2],0);
			break;
			case GEOM_QUAD:
			case GEOM_TRIANGLE:
				//we use triangles for both quad and triangle, 6 vertices per age
				FW_GL_TEXCOORD_POINTER (2,GL_FLOAT,0,(float *)&texcoord[ifloor*2*6],0);
			break;
			default:
			break;
		}
	}
}

void reallyDrawOnce();
void clearDraw();
GLfloat linepts [6] = {-.5f,0.f,0.f, .5f,0.f,0.f};
ushort lineindices[2] = {0,1};
void child_ParticleSystem(struct X3D_ParticleSystem *node){
	// 
	// ParticleSystem 
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/particle_systems.html#ParticleSystem
	// In here we are doing basically what child_Shape does to draw a single geom,
	// except once we send the geometry vertex buffer to the shader,
	// we go into a loop to draw all the particles, sending an xyz position update to the shader
	// for each particle (and color update if colorRamp, or texCoordUpdate if texCoordRamp, and
	//  velocity direction if LINE)
	//
	s_shader_capabilities_t *caps;
	static int once = 0;
	COMPILE_IF_REQUIRED
	if (renderstate()->render_blend == (node->_renderFlags & VF_Blend)) {
	if(node->enabled){
	if(node->isActive){
		int i,j,k,maxparticles;
		double ttime;
		float dtime;
		Stack *_particles;
		struct X3D_Node *tmpNG;
		ttglobal tg = gglobal();

		ttime = TickTime();
		dtime = (float)(ttime - node->_lasttime); //increment to particle age

		if(!once)
			printf("child particlesystem \n");


		//RETIRE remove deceased/retired particles (by packing vector)
		_particles = node->_particles;
		maxparticles = min(node->maxParticles,10000);
		for(i=0,j=0;i<vectorSize(_particles);i++){
			particle pp = vector_get(particle,_particles,i);
			pp.age += dtime;
			if(pp.age < pp.lifespan){
				vector_set(particle,_particles,j,pp); //pack vector to live position j
				j++;
			}
		}

		//PREP PHYSICS - wind geta a per-frame gustiness update
		for(k=0;k<node->physics.n;k++){
			switch(node->physics.p[k]->_nodeType){
				case NODE_WindPhysicsModel:
					prep_windphysics(node->physics.p[k]); break;
				default:
					break;
			}
		}
		//APPLY PHYSICS
		for(i=0;i<vectorSize(_particles);i++){
			particle pp = vector_get(particle,_particles,i);
			float positionChange[3];


			//update velocity vector based on physics accelerations
			// A = F/M
			// V2 = V1 + A*dT
			for(k=0;k<node->physics.n;k++){
				switch(node->physics.p[k]->_nodeType){
					case NODE_WindPhysicsModel:
						apply_windphysics(&pp,node->physics.p[k],dtime); break;
					case NODE_ForcePhysicsModel:
						apply_forcephysics(&pp,node->physics.p[k],dtime); break;
					default:
						break;
				}
			}

			//update position: P1 = P0 + .5(V0 + V1) x dT
			vecscale3f(positionChange,pp.velocity,.5f * dtime);

			for(k=0;k<node->physics.n;k++){
				switch(node->physics.p[k]->_nodeType){
					case NODE_BoundedPhysicsModel:
						apply_boundedphysics(&pp,node->physics.p[k],positionChange); break;
					default:
						break;
				}
			}
			vecadd3f(pp.position,pp.position,positionChange);

			vector_set(particle,_particles,i,pp); //pack vector to live position j
		}

		//CREATE via emitters (implied dtime = 0, so no physics on first frame)
		_particles->n = j;
		if(node->createParticles && _particles->n < maxparticles){
			//create new particles to reach maxparticles limit
			int n_per_frame, n_needed, n_this_frame;
			float particles_per_second, particles_per_frame;
			n_needed = maxparticles - _particles->n;
			//for explosion emitter, we want them all created on the first pass
			//for all the rest we want maxparticles spread over a lifetime, so by the
			//time some start dying, well be at maxparticles
			//particles_per_second [p/s] = maxparticles[p] / particleLifetime[s]
			particles_per_second = (float)node->maxParticles / (float) node->particleLifetime;
			//particles_per_frame [p/f] = particles_per_second [p/s] / frames_per_second [f/s]
			particles_per_frame = particles_per_second * dtime;
			particles_per_frame += node->_remainder;
			n_per_frame = (int)particles_per_frame;
			node->_remainder = particles_per_frame - (float)n_per_frame;
			n_this_frame = min(n_per_frame,n_needed);
			if(node->emitter->_nodeType == NODE_ExplosionEmitter)
				n_this_frame = n_needed;
			j = _particles->n;
			for(i=0;i<n_this_frame;i++,j++){
				particle pp;
				pp.age = 0.0f;
				memset(pp.origin,0,sizeof(float)*3); //for bounded physics
				pp.lifespan = node->particleLifetime * (1.0f + uniformRandCentered()*node->lifetimeVariation);
				memcpy(pp.size,node->particleSize.c,2*sizeof(float));
				//emit particles
				switch(node->emitter->_nodeType){
					case NODE_ConeEmitter:		apply_ConeEmitter(&pp,node->emitter); break;
					case NODE_ExplosionEmitter: apply_ExplosionEmitter(&pp,node->emitter); 
						node->createParticles = FALSE;
						break;
					case NODE_PointEmitter:		apply_PointEmitter(&pp,node->emitter); break;
					case NODE_PolylineEmitter:	apply_PolylineEmitter(&pp,node->emitter); break;
					case NODE_SurfaceEmitter:	apply_SurfaceEmitter(&pp,node->emitter); break;
					case NODE_VolumeEmitter:	apply_VolumeEmitter(&pp,node->emitter); break;
					default:
						break;
				}
				//save particle
				vector_set(particle,_particles,j,pp);
			}
			_particles->n = j;
		}

		//prepare to draw, like child_shape
		//render appearance
		//BORROWED FROM CHILD SHAPE >>>>>>>>>

		int colorSource, alphaSource, isLit, isUserShader; 
		s_shader_capabilities_t *scap;
		//unsigned int shader_requirements;
		shaderflagsstruct shader_requirements;
		memset(&shader_requirements,0,sizeof(shaderflagsstruct));

		//prep_Appearance
		RENDER_MATERIAL_SUBNODES(node->appearance); //child_Appearance


#ifdef HAVE_P
		if (p->material_oneSided != NULL) {
			memcpy (&p->appearanceProperties.fw_FrontMaterial, p->material_oneSided->_verifiedColor.p, sizeof (struct fw_MaterialParameters));
			memcpy (&p->appearanceProperties.fw_BackMaterial, p->material_oneSided->_verifiedColor.p, sizeof (struct fw_MaterialParameters));
			/* copy the emissive colour over for lines and points */
			memcpy(p->appearanceProperties.emissionColour,p->material_oneSided->_verifiedColor.p, 3*sizeof(float));

		} else if (p->material_twoSided != NULL) {
			memcpy (&p->appearanceProperties.fw_FrontMaterial, p->material_twoSided->_verifiedFrontColor.p, sizeof (struct fw_MaterialParameters));
			memcpy (&p->appearanceProperties.fw_BackMaterial, p->material_twoSided->_verifiedBackColor.p, sizeof (struct fw_MaterialParameters));
			/* copy the emissive colour over for lines and points */
			memcpy(p->appearanceProperties.emissionColour,p->material_twoSided->_verifiedFrontColor.p, 3*sizeof(float));
		} else {
			/* no materials selected.... */
		}
#endif

		/* enable the shader for this shape */
		//ConsoleMessage("turning shader on %x",node->_shaderTableEntry);

		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *, node->geometry,tmpNG);

		shader_requirements.base = node->_shaderflags_base; //_shaderTableEntry;  
		shader_requirements.effects = node->_shaderflags_effects;
		shader_requirements.usershaders = node->_shaderflags_usershaders;
		isUserShader = shader_requirements.usershaders ? TRUE : FALSE; // >= USER_DEFINED_SHADER_START ? TRUE : FALSE;
		//if(!p->userShaderNode || !(shader_requirements >= USER_DEFINED_SHADER_START)){
		if(!isUserShader){
			//for Luminance and Luminance-Alpha images, we have to tinker a bit in the Vertex shader
			// New concept of operations Aug 26, 2016
			// in the specs there are some things that can replace other things (but not the reverse)
			// Texture can repace CPV, diffuse and 111
			// CPV can replace diffuse and 111
			// diffuse can replace 111
			// Texture > CPV > Diffuse > (1,1,1)
			// so there's a kind of order / sequence to it.
			// There can be a flag at each step saying if you want to replace the prior value (otherwise modulate)
			// Diffuse replacing or modulating (111) is the same thing, no flag needed
			// Therefore we need at most 2 flags for color:
			// TEXTURE_REPLACE_PRIOR and CPV_REPLACE_PRIOR.
			// and other flag for alpha: ALPHA_REPLACE_PRIOR (same as ! WANT_TEXALPHA)
			// if all those are false, then its full modulation.
			// our WANT_LUMINANCE is really == ! TEXTURE_REPLACE_PRIOR
			// we are missing a CPV_REPLACE_PRIOR, or more precisely this is a default burned into the shader

			int channels;
			//modulation:
			//- for Castle-style full-modulation of texture x CPV x mat.diffuse
			//     and texalpha x (1-mat.trans), set 2
			//- for specs table 17-2 RGB Tex replaces CPV with modulation 
			//     of table 17-2 entries with mat.diffuse and (1-mat.trans) set 1
			//- for specs table 17-3 as written and ignoring modulation sentences
			//    so CPV replaces diffuse, texture replaces CPV and diffuse- set 0
			// testing: KelpForest SharkLefty.x3d has CPV, ImageTexture RGB, and mat.diffuse
			//    29C.wrl has mat.transparency=1 and LumAlpha image, modulate=0 shows sphere, 1,2 inivisble
			//    test all combinations of: modulation {0,1,2} x shadingStyle {gouraud,phong}: 0 looks bright texture only, 1 texture and diffuse, 2 T X C X D
			int modulation = 1; //freewrl default 1 (dug9 Aug 27, 2016 interpretation of Lighting specs)
			channels = getImageChannelCountFromTTI(node->appearance);

			if(modulation == 0)
				shader_requirements.base |= MAT_FIRST; //strict use of table 17-3, CPV can replace mat.diffuse, so texture > cpv > diffuse > 111

			if(shader_requirements.base & COLOUR_MATERIAL_SHADER){
				//printf("has a color node\n");
				//lets turn it off, and see if we get texture
				//shader_requirements &= ~(COLOUR_MATERIAL_SHADER);
				if(modulation == 0) 
					shader_requirements.base |= CPV_REPLACE_PRIOR;
			}

			if(channels && (channels == 3 || channels == 4) && modulation < 2)
				shader_requirements.base |= TEXTURE_REPLACE_PRIOR;
			//if the image has a real alpha, we may want to turn off alpha modulation, 
			// see comment about modulate in Compositing_Shaders.c
			if(channels && (channels == 2 || channels == 4) && modulation == 0)
				shader_requirements.base |= TEXALPHA_REPLACE_PRIOR;

			//getShaderFlags() are from non-leaf-node shader influencers: 
			//   fog, local_lights, clipplane, Effect/EffectPart (for CastlePlugs) ...
			// - as such they may be different for the same shape node DEF/USEd in different branches of the scenegraph
			// - so they are ORd here before selecting a shader permutation
			shader_requirements.base |= getShaderFlags().base; 
			shader_requirements.effects |= getShaderFlags().effects;
			//if(shader_requirements & FOG_APPEARANCE_SHADER)
			//	printf("fog in child_shape\n");

			//ParticleSystem flag
			shader_requirements.base |= PARTICLE_SHADER;
			if(node->colorRamp)
				shader_requirements.base |= HAVE_UNLIT_COLOR;
		}
		//printf("child_shape shader_requirements base %d effects %d user %d\n",shader_requirements.base,shader_requirements.effects,shader_requirements.usershaders);
		scap = getMyShaders(shader_requirements);
		enableGlobalShader(scap);
		//enableGlobalShader (getMyShader(shader_requirements)); //node->_shaderTableEntry));

		//see if we have to set up a TextureCoordinateGenerator type here
		if (tmpNG && tmpNG->_intern) {
			if (tmpNG->_intern->tcoordtype == NODE_TextureCoordinateGenerator) {
				getAppearanceProperties()->texCoordGeneratorType = tmpNG->_intern->texgentype;
				//ConsoleMessage("shape, matprop val %d, geom val %d",getAppearanceProperties()->texCoordGeneratorType, node->geometry->_intern->texgentype);
			}
		}
		//userDefined = (whichOne >= USER_DEFINED_SHADER_START) ? TRUE : FALSE;
		//if (p->userShaderNode != NULL && shader_requirements >= USER_DEFINED_SHADER_START) {
		#ifdef ALLOW_USERSHADERS
		if(isUserShader && p->userShaderNode){
			//we come in here right after a COMPILE pass in APPEARANCE which renders the shader, which sets p->userShaderNode
			//if nothing changed with appearance -no compile pass- we don't come in here again
			//ConsoleMessage ("have a shader of type %s",stringNodeType(p->userShaderNode->_nodeType));
			switch (p->userShaderNode->_nodeType) {
				case NODE_ComposedShader:
					if (X3D_COMPOSEDSHADER(p->userShaderNode)->isValid) {
						if (!X3D_COMPOSEDSHADER(p->userShaderNode)->_initialized) {
							sendInitialFieldsToShader(p->userShaderNode);
						}
					}
					break;
				case NODE_ProgramShader:
					if (X3D_PROGRAMSHADER(p->userShaderNode)->isValid) {
						if (!X3D_PROGRAMSHADER(p->userShaderNode)->_initialized) {
							sendInitialFieldsToShader(p->userShaderNode);
						}
					}

					break;
				case NODE_PackagedShader:
					if (X3D_PACKAGEDSHADER(p->userShaderNode)->isValid) {
						if (!X3D_PACKAGEDSHADER(p->userShaderNode)->_initialized) {
							sendInitialFieldsToShader(p->userShaderNode);
						}
					}

					break;
			}
		}
		#endif //ALLOW_USERSHADERS
		//update effect field uniforms
		if(shader_requirements.effects){
			update_effect_uniforms();
		}

		//<<<<< BORROWED FROM CHILD SHAPE


		//send materials, textures, matrices to shader
		textureTransform_start();
		setupShaderB();
		//send vertex buffer to shader
		int allowsTexcoordRamp = FALSE;
		float *texcoord = NULL;
		switch(node->_geometryType){
			case GEOM_LINE: 
			{
				FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(float *)linepts);
				sendElementsToGPU(GL_LINES,2,(ushort *)lineindices);
				texcoord = (float*)node->_ltex;
				allowsTexcoordRamp = TRUE;
			}
			break;
			case GEOM_POINT: 
			{
				float point[3];
				memset(point,0,3*sizeof(float));
				FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(GLfloat *)point);
        		sendArraysToGPU (GL_POINTS, 0, 1);
			}
			break;
			case GEOM_QUAD: 
			{
				//textureCoord_send(&mtf);
				FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(GLfloat *)node->_tris);
				FW_GL_NORMAL_POINTER (GL_FLOAT,0,twotrisnorms);
				sendArraysToGPU (GL_TRIANGLES, 0, 6);
				texcoord = (float*)node->_ttex;
				allowsTexcoordRamp = TRUE;
			}
			break;
			case GEOM_SPRITE: 
			{
				//textureCoord_send(&mtf);
				FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(GLfloat *)node->_tris);
				FW_GL_NORMAL_POINTER (GL_FLOAT,0,twotrisnorms);
				sendArraysToGPU (GL_TRIANGLES, 0, 6);
			}
			break;
			case GEOM_TRIANGLE: 
			{
				//textureCoord_send(&mtf);
				FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(GLfloat *)node->_tris);
				FW_GL_NORMAL_POINTER (GL_FLOAT,0,twotrisnorms);
				sendArraysToGPU (GL_TRIANGLES, 0, 6);
				texcoord = (float*)node->_ttex;
				allowsTexcoordRamp = TRUE;
			}
			break;
			case GEOM_GEOMETRY: 
				render_node(node->geometry);
			break;
			default:
				break;
		}

		GLint ppos = GET_UNIFORM(scap->myShaderProgram,"particlePosition");
		GLint cr = GET_UNIFORM(scap->myShaderProgram,"fw_UnlitColor");
		GLint gtype = GET_UNIFORM(scap->myShaderProgram,"fw_ParticleGeomType");
		glUniform1i(gtype,node->_geometryType); //for SPRITE = 4, screen alignment
		//loop over live particles, drawing each one
		int haveColorRamp = node->colorRamp ? TRUE : FALSE;
		haveColorRamp = haveColorRamp && cr > -1;
		int haveTexcoordRamp = node->texCoordRamp ? TRUE : FALSE;
		haveTexcoordRamp = haveTexcoordRamp && allowsTexcoordRamp && texcoord; 

		for(i=0;i<vectorSize(_particles);i++){
			particle pp = vector_get(particle,_particles,i);
			//update particle-specific uniforms
			glUniform3fv(ppos,1,pp.position);
			if(haveColorRamp)
				updateColorRamp(node,&pp,cr);
			if(haveTexcoordRamp)
				updateTexCoordRamp(node,&pp,texcoord);
			if(node->_geometryType == GEOM_LINE){
				float lpts[6], vel[3];
				vecnormalize3f(vel,pp.velocity);
				vecscale3f(&lpts[3],vel,.5*node->particleSize.c[1]);
				vecscale3f(&lpts[0],vel,-.5*node->particleSize.c[1]);
				FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,(float *)lpts);
			}
			//draw
			reallyDrawOnce();
		}
		clearDraw();
		//cleanup after draw, like child_shape
		FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
		FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);
		textureTransform_end();

		//BORROWED FROM CHILD_SHAPE >>>>>>
		//fin_Appearance
		if(node->appearance){
			struct X3D_Appearance *tmpA;
			POSSIBLE_PROTO_EXPANSION(struct X3D_Appearance *,node->appearance,tmpA);
			if(tmpA->effects.n)
				fin_sibAffectors(X3D_NODE(tmpA),&tmpA->effects);
		}
		/* any shader turned on? if so, turn it off */

		//ConsoleMessage("turning shader off");
		finishedWithGlobalShader();
#ifdef HAVE_P
		p->material_twoSided = NULL;
		p->material_oneSided = NULL;
		p->userShaderNode = NULL;
#endif
		tg->RenderFuncs.shapenode = NULL;
    
		/* load the identity matrix for textures. This is necessary, as some nodes have TextureTransforms
			and some don't. So, if we have a TextureTransform, loadIdentity */
    
#ifdef HAVE_P
		if (p->this_textureTransform) {
			p->this_textureTransform = NULL;
#endif //HAVE_P
			FW_GL_MATRIX_MODE(GL_TEXTURE);
			FW_GL_LOAD_IDENTITY();
			FW_GL_MATRIX_MODE(GL_MODELVIEW);
#ifdef HAVE_P
		}
#endif    
		/* LineSet, PointSets, set the width back to the original. */
		{
			float gl_linewidth = tg->Mainloop.gl_linewidth;
			glLineWidth(gl_linewidth);
#ifdef HAVE_P
			p->appearanceProperties.pointSize = gl_linewidth;
#endif
		}

		/* did the lack of an Appearance or Material node turn lighting off? */
		LIGHTING_ON;

		/* turn off face culling */
		DISABLE_CULL_FACE;

		//<<<<< BORROWED FROM CHILD_SHAPE

		node->_lasttime = ttime;
	} //isActive
	} //enabled
	} //VF_Blend
	once = 1;
}
