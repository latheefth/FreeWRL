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
#include <iglobal.h>
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
	int C;
    destination = node->initialDestination;

    Buffer_length= cNumSupports;

    Buffer[0]= node->initialDestination; //initial_destination;
    for(C= 1; C<Buffer_length; C++ )
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

double UpdateBuffer(struct X3D_PositionChaser *node, double Now)
{
	int C;
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

            for( C= Buffer_length - 1; C>=NumToShift; C-- )
                Buffer[C]= Buffer[C - NumToShift];

            for( C= 0; C<NumToShift; C++ )
            {
                // Hmm, we have a destination value, but don't know how it has
                // reached the current state.
                // Therefore we do a linear interpolation from the latest value in the buffer to destination.

				float tmp1[3],tmp2[3];
                float Alpha= (float)C / (float)NumToShift;
				// might need to chain functions like this backward:
				// float *vecadd3f(float *c, float *a, float *b)
				// and feed it temps in the *c variable
				// and in the last step use Buffer[] as *c
                //Buffer[C]= Buffer[NumToShift].multiply(Alpha).add(destination.multiply((1 - Alpha)));
				//vecadd3f(Buffer[C].c,vecscale3f(Buffer[NumToShift].c,(float)Alpha),vecscale3f(tmp2,destination.c,(1.0f-Alpha));
				vecadd3f(Buffer[C].c,vecscale3f(tmp1,Buffer[NumToShift].c,Alpha),vecscale3f(tmp2,destination.c,1.0f - Alpha));
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

            for( C= 0; C<Buffer_length; C++ )
                Buffer[C]= destination;
        }

        BufferEndTime+= NumToShift * cStepTime;
    }
    return Frac;
}
//when a route toNode.toField is PositionChaser.set_destination
//we need to call this function (somehow) much like a script?
//
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

double StepResponse(double duration)
{
	double t = TickTime();
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
	int C;
	double Frac;
    struct SFVec3f Output;
    struct SFVec3f DeltaIn;
    struct SFVec3f DeltaOut;

	CheckInit(node);

    if(!BufferEndTime)
    {
        BufferEndTime= Now; // first event we received, so we are in the initialization phase.

        node->value_changed= node->initialValue; //initial_value;
        return;
    }

    Frac= UpdateBuffer(node, Now);
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

    Output= previousValue;
    //DeltaIn= Buffer[Buffer_length - 1].subtract(previousValue);
	vecdif3f(DeltaIn.c,Buffer[Buffer_length - 1].c,previousValue.c);
    //DeltaOut= DeltaIn.multiply(StepResponse((Buffer_length - 1 + Frac) * cStepTime));
	vecscale3f(DeltaOut.c,DeltaIn.c,(float)StepResponse((Buffer_length - 1 + Frac) * cStepTime));
    //Output= Output.add(DeltaOut);
	vecadd3f(Output.c,Output.c,DeltaOut.c);

    for(C= Buffer_length - 2; C>=0; C-- )
    {
        //DeltaIn= Buffer[C].subtract(Buffer[C + 1]);
		vecdif3f(DeltaIn.c,Buffer[C].c,Buffer[C+1].c);

        //DeltaOut= DeltaIn.multiply(StepResponse((C + Frac) * cStepTime));
		vecscale3f(DeltaOut.c,DeltaIn.c,(float)StepResponse(((double)C + Frac) * cStepTime));

        //Output= Output.add(DeltaOut);
		vecadd3f(Output.c,Output.c,DeltaOut.c);
    }
    //if(Output != node->value_changed)
	if(!vecsame3f(Output.c,node->value_changed.c)){
        node->value_changed= Output;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, value_changed));
		printf("+");
	}

}



void do_PositionChaserTick(void * ptr){
	double Now;
	struct X3D_PositionChaser *node = (struct X3D_PositionChaser *)ptr;
	if(!node) return;
	Now = TickTime();
	if(!vecsame3f(node->set_destination.c,previousValue.c))
		set_destination(node, node->set_destination,Now);
	Tick(node,Now);
	printf(".");

}

/*
	//positiondamper
    <ProtoDeclare name='PositionDamper'>
	//public:
          <field accessType='inputOnly' name='set_value' type='SFVec3f'/>
          <field accessType='initializeOnly' name='reachThreshold' type='SFFloat'/>  //tolerance?
          <field accessType='initializeOnly' name='input' type='SFVec3f'/>
          <field accessType='initializeOnly' name='eps' type='SFFloat'/>   //????
          <field accessType='inputOnly' name='set_destination' type='SFVec3f'/>
          <field accessType='outputOnly' name='value_changed' type='SFVec3f'/>
		  //tau >>
          <field accessType='initializeOnly' name='tau' type='SFFloat' value='1.0'/>
          <field accessType='inputOnly' name='set_tau' type='SFFloat'/>
          <field accessType='initializeOnly' name='effs' type='SFNode'>  //???? not sure why they did this _and_ tau
            <ProtoInstance USE='EFFS' name='EFFS'/>
          </field>
		  //<<tau
          <field accessType='initializeOnly' name='order' type='SFInt32'/>
          <field accessType='initializeOnly' name='initial_value' type='SFVec3f'/>
          <field accessType='outputOnly' name='reached' type='SFBool'/>       //isActive?
          <field accessType='initializeOnly' name='takeFirstInput' type='SFBool'/>  //compensation for no separate initialDestination??
          <IS>
            <connect nodeField='set_value' protoField='set_value'/>
            <connect nodeField='reachThreshold' protoField='reachThreshold'/>
            <connect nodeField='input' protoField='initial_destination'/>
            <connect nodeField='eps' protoField='eps'/>
            <connect nodeField='set_destination' protoField='set_destination'/>
            <connect nodeField='value_changed' protoField='value_changed'/>
            <connect nodeField='order' protoField='order'/>
            <connect nodeField='initial_value' protoField='initial_value'/>
            <connect nodeField='reached' protoField='reached'/>
            <connect nodeField='takeFirstInput' protoField='takeFirstInput'/>
          </IS>
		  //private
          <field accessType='initializeOnly' name='bInitialized' type='SFBool' value='false'/>
          <field accessType='initializeOnly' name='lastTick' type='SFTime' value='0.0'/>
          <field accessType='initializeOnly' name='bNeedToTakeFirstInput' type='SFBool' value='true'/>
          <field accessType='initializeOnly' name='value5' type='SFVec3f' value='0.0 0.0 0.0'/>
          <field accessType='initializeOnly' name='value4' type='SFVec3f' value='0.0 0.0 0.0'/>
          <field accessType='initializeOnly' name='value3' type='SFVec3f' value='0.0 0.0 0.0'/>
          <field accessType='initializeOnly' name='value2' type='SFVec3f' value='0.0 0.0 0.0'/>
          <field accessType='initializeOnly' name='value1' type='SFVec3f' value='0.0 0.0 0.0'/>
*/
static int bInitializedD = FALSE;
static double lastTick = 0.0;
static int bNeedToTakeFirstInput = TRUE;
static struct SFVec3f value5 = {0.0f,0.0f,0.0f};
static struct SFVec3f value4 = {0.0f,0.0f,0.0f};
static struct SFVec3f value3 = {0.0f,0.0f,0.0f};
static struct SFVec3f value2 = {0.0f,0.0f,0.0f};
static struct SFVec3f value1 = {0.0f,0.0f,0.0f};
static struct SFVec3f input = {0.0f,0.0f,0.0f};



void set_valueD(struct X3D_PositionDamper *node, struct SFVec3f opos);
void InitD(struct X3D_PositionDamper *node)
{
    bNeedToTakeFirstInput= TRUE;

    //tau= node->tau;
    set_valueD(node,node->initialValue); //initial_value);
    //if(IsCortona)
    //    needTimer= true;
    //else
    //    needTimer=    input.x != initial_value.x
    //               || input.y != initial_value.y
    //               || input.z != initial_value.z
    //               ;
}
void CheckInitD(struct X3D_PositionDamper *node)
{
    if(!bInitializedD)
    {
        bInitializedD= TRUE;
        InitD(node);
    }

}

void set_valueD(struct X3D_PositionDamper *node, struct SFVec3f opos)
{
    CheckInitD(node);

    bNeedToTakeFirstInput= false;

    value1= value2= value3= value4= value5= opos;
    node->value_changed= opos;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));

    UpdateReached(node);
    //StartTimer();
	node->isActive = TRUE;
}

void set_destinationD(struct X3D_PositionDamper *node, struct SFVec3f ipos)
{
    CheckInitD(node);

    if(bNeedToTakeFirstInput)
    {
        bNeedToTakeFirstInput= FALSE;
        set_valueD(node,ipos);
    }


    if(!vecsame3f(ipos.c,input.c))
    {
        input = ipos;
        //StartTimer();
		node->isActive = TRUE;
    }
}

float GetDist(struct X3D_PositionDamper *node)
{
	float tmp[3];
    //double dist= value1.subtract(node->initialDestination).length();
	float dist = veclength3f(vecdif3f(tmp,value1.c,input.c));
    if(node->order > 1)
    {
        //double dist2= value2.subtract(value1).length();
		float dist2 = veclength3f(vecdif3f(tmp,value2.c,value1.c));
        if( dist2 > dist)  dist= dist2;
    }
    if(node->order > 2)
    {
        //double dist3= value3.subtract(value2).length();
		float dist3 = veclength3f(vecdif3f(tmp,value3.c,value2.c));
        if( dist3 > dist)  dist= dist3;
    }
    if(node->order > 3)
    {
        //double dist4= value4.subtract(value3).length();
		float dist4 = veclength3f(vecdif3f(tmp,value4.c,value3.c));
        if( dist4 > dist)  dist= dist4;
    }
    if(node->order > 4)
    {
        //double dist5= value5.subtract(value4).length();
		float dist5 = veclength3f(vecdif3f(tmp,value5.c, value4.c));
        if( dist5 > dist)  dist= dist5;
    }
    return dist;
}
struct SFVec3f diftimes(struct SFVec3f a, struct SFVec3f b, double alpha){
	struct SFVec3f ret;
	float tmp[3], tmp2[3];
	//input  .add(value1.subtract(input  ).multiply(alpha))
	vecadd3f(ret.c,a.c,vecscale3f(tmp2,vecdif3f(tmp,b.c,a.c),alpha));
	return ret;
}
void tick_positiondamper(struct X3D_PositionDamper *node, double now)
{
	struct SFVec3f tmp;
    CheckInitD(node);

    if(!lastTick)
    {
        lastTick= now;
        return;
    }

    double delta= now - lastTick;
    lastTick= now;

    double alpha= exp(-delta / node->tau);


    if(bNeedToTakeFirstInput)  // then don't do any processing.
        return;

    value1= node->order > 0 && node->tau != 0.0
               //? input  .add(value1.subtract(input  ).multiply(alpha))
			   ? diftimes(input,value1,alpha)
               : input;

    value2= node->order > 1 && node->tau != 0.0
               //? value1.add(value2.subtract(value1).multiply(alpha))
			   ? diftimes(value1,value2,alpha)
               : value1;

    value3= node->order > 2 && node->tau != 0.0
               //? value2.add(value3.subtract(value2).multiply(alpha))
			   ? diftimes(value2,value3,alpha)
               : value2;

    value4= node->order > 3 && node->tau != 0.0
               //? value3.add(value4.subtract(value3).multiply(alpha))
			   ? diftimes(value3,value4,alpha)
               : value3;

    value5= node->order > 4 && node->tau != 0.0
               //? value4.add(value5.subtract(value4).multiply(alpha))
			   ? diftimes(value4,value5,alpha)
               : value4;

    float dist= GetDist(node);

    if(dist < node->tolerance) //eps)
    {
        value1= value2= value3= value4= value5= input;

        node->value_changed= input;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));

        UpdateReached2(node,dist);

       // StopTimer();
	    node->isActive = FALSE;
        return;
    }
    node->value_changed= value5;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));

    UpdateReached2(node,dist);

}

static int reached = FALSE;
int UpdateReached2(struct X3D_PositionDamper *node, float Dist)
{
    if(reached)
    {
        if(Dist > node->tolerance) //reachThreshold)
            reached= FALSE;
    }else
    {
        if(Dist <= node->tolerance) //reachThreshold)
            reached= TRUE;
    }
	return reached;
}
int UpdateReached(struct X3D_PositionDamper *node)
{
    return UpdateReached2(node,GetDist(node));
}



void do_PositionDamperTick(void * ptr){
	struct X3D_PositionDamper *node = (struct X3D_PositionDamper *)ptr;
	if(!node)return;
	if(!vecsame3f(node->set_destination.c,previousValue.c))
		set_destinationD(node, node->set_destination);
	tick_positiondamper(node,TickTime());
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


