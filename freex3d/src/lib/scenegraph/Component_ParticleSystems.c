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
#include "LinearAlgebra.h"
//#include "Component_ParticleSystems.h"
#include "Children.h"

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
*/


float uniformRand(){
	// 0 to 1 inclusive
	static int once = 0;
	unsigned int ix;
	float rx;

	if(!once)
		srand((unsigned int) Time1970sec());
	once = 1;
	ix = rand();  
	rx = (float)ix/(float)RAND_MAX; //you would do (RAND_MAX - 1) here for excluding 1
	return rx;
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

typedef struct {
	//store at end of current iteration, for use on next iteration
	double lifetimeRemaining;
	float position[3];
	float velocity[3];
} particle;

GLfloat quadtris [18] = {1.0f,1.0f,0.0f, -1.0f,1.0f,0.0f, -1.0f,-1.0f,0.0f,    1.0f,1.0f,0.0f, -1.0f,-1.0f,0.0f, 1.0f,-1.0f,0.0f};
void compile_ParticleSystem(struct X3D_ParticleSystem *node){
	int i,j;
	float *boxtris;

	ConsoleMessage("compile_particlesystem\n");
	if(node->_tris == NULL){
		node->_tris = MALLOC(void *,18 * sizeof(float));
		memcpy(node->_tris,quadtris,18*sizeof(float));
	}
	MARK_NODE_COMPILED
}

void child_ParticleSystem(struct X3D_ParticleSystem *node){
	// 
	// ParticleSystem 
	s_shader_capabilities_t *caps;
	static int once = 0;
	COMPILE_IF_REQUIRED

	if (renderstate()->render_blend == (node->_renderFlags & VF_Blend)) {
		if(!once)
			printf("child particlesystem \n");
	} //VF_Blend
	once = 1;
}
