/*


X3D Followers Component

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
//#include "Component_Followers.h"
#include "Children.h"


typedef struct pComponent_Followers{
	int something;
}* ppComponent_Followers;
void *Component_Followers_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Followers));
	memset(v,0,sizeof(struct pComponent_Followers));
	return v;
}
void Component_Followers_init(struct tComponent_Followers *t){
	//public
	//private
	t->prv = Component_Followers_constructor();
	{
		ppComponent_Followers p = (ppComponent_Followers)t->prv;
		p->something = 0;
	}
}
void Component_Followers_clear(struct tComponent_Followers *t){
	//public
}

//ppComponent_Followers p = (ppComponent_Followers)gglobal()->Component_Followers.prv;

void do_ColorChaserTick(void * ptr);
void do_ColorDamperTick(void * ptr);
void do_CoordinateChaserTick(void * ptr);
void do_CoordinateDamperTick(void * ptr);
void do_OrientationChaserTick(void * ptr);
void do_OrientationDamperTick(void * ptr);
void do_PositionChaserTick(void * ptr);
void do_ColorDamperTick(void * ptr);
void do_PositionChaserTick(void * ptr);
void do_PositionDamperTick(void * ptr);
void do_PositionChaser2DTick(void * ptr);
void do_PositionDamper2DTick(void * ptr);
void do_ScalarChaserTick(void * ptr);
void do_ScalarDamperTick(void * ptr);
void do_TexCoordChaser2DTick(void * ptr);
void do_TexCoordDamper2DTick(void * ptr);

void do_ColorChaserTick(void * ptr){
	struct X3D_ColorChaser *node = (struct X3D_ColorChaser *)ptr;
	if(!node)return;

}
void do_ColorDamperTick(void * ptr){
	struct X3D_ColorDamper *node = (struct X3D_ColorDamper *)ptr;
	if(!node)return;

}

void do_CoordinateChaserTick(void * ptr){
	struct X3D_CoordinateChaser *node = (struct X3D_CoordinateChaser *)ptr;
	if(!node)return;

}
void do_CoordinateDamperTick(void * ptr){
	struct X3D_CoordinateDamper *node = (struct X3D_CoordinateDamper *)ptr;
	if(!node)return;

}

void do_OrientationChaserTick(void * ptr){
	struct X3D_OrientationChaser *node = (struct X3D_OrientationChaser *)ptr;
	if(!node)return;

}
void do_OrientationDamperTick(void * ptr){
	struct X3D_OrientationDamper *node = (struct X3D_OrientationDamper *)ptr;
	if(!node)return;

}

void do_PositionChaserTick(void * ptr){
	struct X3D_PositionChaser *node = (struct X3D_PositionChaser *)ptr;
	if(!node)return;
	//printf(".");

}
void do_PositionDamperTick(void * ptr){
	struct X3D_PositionDamper *node = (struct X3D_PositionDamper *)ptr;
	if(!node)return;
	//printf("!");
}

void do_PositionChaser2DTick(void * ptr){
	struct X3D_PositionChaser2D *node = (struct X3D_PositionChaser2D *)ptr;
	if(!node)return;

}
void do_PositionDamper2DTick(void * ptr){
	struct X3D_PositionDamper2D *node = (struct X3D_PositionDamper2D *)ptr;
	if(!node)return;

}

void do_ScalarChaserTick(void * ptr){
	struct X3D_ScalarChaser *node = (struct X3D_ScalarChaser *)ptr;
	if(!node)return;

}
void do_ScalarDamperTick(void * ptr){
	struct X3D_ScalarDamper *node = (struct X3D_ScalarDamper *)ptr;
	if(!node)return;

}

void do_TexCoordChaser2DTick(void * ptr){
	struct X3D_TexCoordChaser2D *node = (struct X3D_TexCoordChaser2D *)ptr;
	if(!node)return;

}
void do_TexCoordDamper2DTick(void * ptr){
	struct X3D_TexCoordDamper2D *node = (struct X3D_TexCoordDamper2D *)ptr;
	if(!node)return;

}

//============================


