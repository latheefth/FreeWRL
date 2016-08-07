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
		


*/