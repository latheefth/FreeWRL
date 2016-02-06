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

/*
	adapted from
	http://www.web3d.org/x3d/content/examples/Basic/Followers/index.html
    PrototypeDeclaration scene
		  <field accessType='inputOnly' name='Tick' type='SFTime'/>

		  I interpret these as variables on the public interface of the node:
          <field accessType='inputOnly' name='set_value' type='SFVec3f'/>
          <field accessType='initializeOnly' name='duration' type='SFTime'/>
          <field accessType='inputOnly' name='set_destination' type='SFVec3f'/>
          <field accessType='outputOnly' name='value_changed' type='SFVec3f'/>
          <field accessType='initializeOnly' name='initial_destination' type='SFVec3f'/>
          <field accessType='outputOnly' name='isActive' type='SFBool'/>
          <field accessType='initializeOnly' name='initial_value' type='SFVec3f'/>
          <IS>
            <connect nodeField='set_value' protoField='set_value'/>
            <connect nodeField='duration' protoField='duration'/>
            <connect nodeField='set_destination' protoField='set_destination'/>
            <connect nodeField='value_changed' protoField='value_changed'/>
            <connect nodeField='initial_destination' protoField='initial_destination'/>
            <connect nodeField='isActive' protoField='isActive'/>
            <connect nodeField='initial_value' protoField='initial_value'/>
          </IS>

		  I interpret these as variables private to the node or component:
          <field accessType='initializeOnly' name='Buffer' type='MFVec3f'/>
          <field accessType='initializeOnly' name='bInitialized' type='SFBool' value='false'/>
          <field accessType='initializeOnly' name='BufferEndTime' type='SFTime' value='0.0'/>
          <field accessType='initializeOnly' name='cNumSupports' type='SFInt32' value='10'/>
          <field accessType='initializeOnly' name='cStepTime' type='SFTime' value='0.0'/>
          <field accessType='initializeOnly' name='previousValue' type='SFVec3f' value='0.0 0.0 0.0'/>
          <field accessType='initializeOnly' name='destination' type='SFVec3f' value='0.0 0.0 0.0'/>
*/

/*
//for now we'll put the private variables static, 
// but they would go into a _private struct field in the node
static int Buffer_length = 0;
static struct SFVec3f Buffer[10];
static int bInitialized = 0;
static double BufferEndTime = 0.0;
static int cNumSupports = 10;
static double cStepTime = 0.0;
static struct SFVec3f previousValue = { 0.0f, 0.0f, 0.0f};
static struct SFVec3f destination = {0.0f,0.0f,0.0f};
void Init(struct X3D_PositionChaser *node)
{
    destination = node->initialDestination;

    Buffer_length= cNumSupports;

    Buffer[0]= node->initialDestination; //initial_destination;
    for(int C= 1; C<Buffer_length; C++ )
        Buffer[C]= node->initialValue; //initial_value;

    previousValue= node->initialValue; //initial_value;

    cStepTime= node->duration / (double) cNumSupports;
}
void CheckInit(struct X3D_PositionChaser *node)
{
    if(!bInitialized)
    {
        bInitialized= TRUE;  // Init() may call other functions that call CheckInit(). In that case it's better the flag is already set, otherwise an endless loop would occur.
        Init(node);
    }
}

double UpdateBuffer(struct X3D_PositionChaser *node, double Now, double BufferEndTime, double cStepTime)
{
    double Frac= (Now - BufferEndTime) / cStepTime;
    // is normally < 1. When it has grown to be larger than 1, we have to shift the array because the step response
    // of the oldest entry has already reached its destination, and it's time for a newer entry.
    // has already reached it
    // In the case of a very low frame rate, or a very short cStepTime we may need to shift by more than one entry.

    if(Frac >= 1.0)
    {
        int NumToShift= (int)floor(Frac);
        Frac-= (double) NumToShift;

        if(NumToShift < Buffer_length)
        {   // normal case.

            previousValue= Buffer[Buffer_length - NumToShift];

            for(int C= Buffer_length - 1; C>=NumToShift; C-- )
                Buffer[C]= Buffer[C - NumToShift];

            for(int C= 0; C<NumToShift; C++ )
            {
                // Hmm, we have a destination value, but don't know how it has
                // reached the current state.
                // Therefore we do a linear interpolation from the latest value in the buffer to destination.

                int Alpha= C / NumToShift;
				// might need to chain functions like this backward:
				// float *vecadd3f(float *c, float *a, float *b)
				// and feed it temps in the *c variable
				// and in the last step use Buffer[] as *c
                Buffer[C]= Buffer[NumToShift].multiply(Alpha).add(destination.multiply((1 - Alpha)));
            }
        }else
        {
            // degenerated case:
            //
            // We have a _VERY_ low frame rate...
            // we can only guess how we should fill the array.
            // Maybe we could write part of a linear interpolation
            // from Buffer[0] to destination, that goes from BufferEndTime to Now
            // (possibly only the end of the interpolation is to be written),
            // but if we rech here we are in a very degenerate case...
            // Thus we just write destination to the buffer.

            previousValue= NumToShift == Buffer_length? Buffer[0] : destination;

            for(int C= 0; C<Buffer_length; C++ )
                Buffer[C]= destination;
        }

        BufferEndTime+= NumToShift * cStepTime;
    }
    return Frac;
}
void set_destination(struct X3D_PositionChaser *node, struct SFVec3f Dest, double Now)
{
    CheckInit(node);

    destination= Dest;
    // Somehow we assign to Buffer[-1] and wait untill this gets shifted into the real buffer.
    // Would we assign to Buffer[0] instead, we'd have no delay, but this would create a jump in the
    // output because Buffer[0] is associated with a value in the past.

    UpdateBuffer(node, Now);
}
// This function defines the shape of how the output responds to the input.
// It must accept values for T in the range 0 <= T <= 1.
// In order to create a smooth animation, it should return 0 for T == 0,
// 1 for T == 1 and be sufficient smooth in the range 0 <= T <= 1.

// It should be optimized for speed, in order for high performance. It's
// executed Buffer.length + 1 times each simulation tick.

double StepResponseCore(double T)
{
    return .5 - .5 * cos(T * PI);
}

double StepResponse(double t, double duration)
{
    if(t < 0.0)
        return 0.0;

    if(t > duration)
        return 1.0;

    // When optimizing for speed, the above two if(.) cases can be omitted,
    // as this funciton will not be called for values outside of 0..duration.

    return StepResponseCore(t / duration);
}

void Tick(struct X3D_PositionChaser *node, double Now)
{
    CheckInit(node);

    if(!BufferEndTime)
    {
        BufferEndTime= Now; // first event we received, so we are in the initialization phase.

        node->value_changed= node->initialValue; //initial_value;
        return;
    }

    double Frac= UpdateBuffer(node, Now);
    // Frac is a value in   0 <= Frac < 1.

    // Now we can calculate the output.
    // This means we calculate the delta between each entry in Buffer and its previous
    // entries, calculate the step response of each such step and add it to form the output.

    // The oldest vaule Buffer[Buffer.length - 1] needs some extra thought, because it has
    // no previous value. More exactly, we haven't stored a previous value anymore.
    // However, the step response of that missing previous value has already reached its
    // destination, so we can - would we have that previous value - use this as a start point
    // for adding the step responses.
    // Actually UpdateBuffer(.) maintains this value in

    struct SFVec3f Output= previousValue;

    struct SFVec3f DeltaIn= Buffer[Buffer_length - 1].subtract(previousValue);

    struct SFVec3f DeltaOut= DeltaIn.multiply(StepResponse((Buffer_length - 1 + Frac) * cStepTime));

    Output= Output.add(DeltaOut);

    for(int C= Buffer_length - 2; C>=0; C-- )
    {
        struct SFVec3f DeltaIn= Buffer[C].subtract(Buffer[C + 1]);

        struct SFVec3f DeltaOut= DeltaIn.multiply(StepResponse((C + Frac) * cStepTime));

        Output= Output.add(DeltaOut);
    }
    if(Output != node->value_changed)
        node->value_changed= Output;
}

*/

void do_PositionChaserTick(void * ptr){
	double Now;
	struct X3D_PositionChaser *node = (struct X3D_PositionChaser *)ptr;
	if(!node)return;
	Now = TickTime();
	//Tick(node,Now);
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


