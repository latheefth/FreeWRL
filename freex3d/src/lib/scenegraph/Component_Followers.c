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
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		veccopy3f(node->value_changed.c, node->set_destination.c);
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_ColorChaser, value_changed));
		MARK_NODE_COMPILED
	}
}
void do_ColorDamperTick(void * ptr){
	struct X3D_ColorDamper *node = (struct X3D_ColorDamper *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		veccopy3f(node->value_changed.c, node->set_destination.c);
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_ColorDamper, value_changed));
		MARK_NODE_COMPILED
	}
}

void do_CoordinateChaserTick(void * ptr){
	struct X3D_CoordinateChaser *node = (struct X3D_CoordinateChaser *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		int n;
		n = node->set_destination.n;
		node->value_changed.n = n;
		node->value_changed.p = realloc(node->value_changed.p,n * sizeof(struct SFVec3f));
		memcpy(node->value_changed.p,node->set_destination.p,n*sizeof(struct SFVec3f));
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_CoordinateChaser, value_changed));
		MARK_NODE_COMPILED
	}
}
void do_CoordinateDamperTick(void * ptr){
	struct X3D_CoordinateDamper *node = (struct X3D_CoordinateDamper *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		int n;
		n = node->set_destination.n;
		node->value_changed.n = n;
		node->value_changed.p = realloc(node->value_changed.p,n * sizeof(struct SFVec3f));
		memcpy(node->value_changed.p,node->set_destination.p,n*sizeof(struct SFVec3f));
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_CoordinateDamper, value_changed));
		MARK_NODE_COMPILED
	}
}

void do_OrientationChaserTick(void * ptr){
	struct X3D_OrientationChaser *node = (struct X3D_OrientationChaser *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		veccopy3f(node->value_changed.c, node->set_destination.c);
		node->value_changed.c[3] = node->set_destination.c[3];
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_OrientationChaser, value_changed));
		MARK_NODE_COMPILED
	}
}
void do_OrientationDamperTick(void * ptr){
	struct X3D_OrientationDamper *node = (struct X3D_OrientationDamper *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		veccopy3f(node->value_changed.c, node->set_destination.c);
		node->value_changed.c[3] = node->set_destination.c[3];
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_OrientationDamper, value_changed));
		MARK_NODE_COMPILED
	}
}

/*
	adapted from
	http://www.web3d.org/x3d/content/examples/Basic/Followers/index.html
*/


//for now we'll put the private variables static, 
// but they would go into a _private struct field in the node
static int Buffer_length = 10;
//static int cNumSupports = 10;
//static int bInitialized = 0;
//static double BufferEndTime = 0.0;
//static double cStepTime = 0.0;
//static struct SFVec3f previousValue = { 0.0f, 0.0f, 0.0f};
//static struct SFVec3f destination = {0.0f,0.0f,0.0f};
//static struct SFVec3f Buffer[10];

/*
//in theory you could have something like this, 
// and some generalized functions that delegate to some
// function pointers or switch_case on type of element.
//global constants
	static int Buffer_length = 10;
	static int cNumSupports = 10;

//common, can initialize to common
typedef struct chaser_struct {
	int bInitialized;
	double BufferEndTime;
	double cStepTime;
	//offsetofs
	void *value_changed;
	void *isActive;
	void *set_destination;
	void *set_value;
	//privates
	void *previousValue;
	void *destination;
	void *Buffer;
}chaser_struct;
typedef struct chaser_data {
struct SFVec3f previousValue;
struct SFVec3f destination;
struct SFVec3f Buffer[10];
}chaser_data;

There are a number of ways to abstract types for generic algorithms in C
a. ## macroization - used frequently in freewrl, however MF and SF still need different treatement
	can go SFa = SFb, but MFa = MFb won't deep copy the p*
	similar to templates in C++, generates separate code for each type during compilation
b. low-level functions like ADD have a switch-case on fieldtype (need to pass in field type)
c. function pointers that know the type, to abstract handling of an opaque type
I'll try a combination here, as an experiment.
A. keeping a certain order to the fields:
- non-field-type-specific fields come first -int, bool, time- then can use a generic node->fieldname
- value_changed the first field-type-specific field so offsetof(,value_changed) is the same for all chasers (and dampers)
B. using c. above, generic functions for handling opaque types, with functions knowing the type

*/

//goal: prepare the absolute addresses of field-type-specific fields
// for chaser and damper to generisize algos without using ## macros and offsetof
// which compiles to more code
// the following _ptrs structs would be populated in do_ on initialization
// for each node instance (since absolute pointers vary with instance)
typedef struct chaser_ptrs {
	//public
	void *value_changed;
	void *initialDestination;
	void *initialValue;
	void *set_destination;
	void *set_value;
	//private
	void *_buffer;
	void *_previousValue;
	void *_destination;
}chaser_ptrs;
typedef struct damper_ptrs {
	//public
	void *value_changed;
	void *initialDestination;
	void *initialValue;
	void *set_destination;
	void *set_value;
	//private
	void *_values;
	void *_input;
} damper_ptrs;

//goal: abstract a few numerical field types without using ## macroization
//- including SF and MF in same interface, so algos can be generic
//  this should be static for a fieldtype
typedef struct ftype {
	void* (*copy)(void *T,void *A);
	void* (*add)(void *T,void* A,void* B);
	void* (*dif)(void *T,void* A,void* B);
	void* (*scale)(void *T,void* A,float S);
	float (*dist)(void* A);
	int (*same)(void* A,void* B);
	int (*approx)(void* A,void* B);
	void* (*arr)(void* A,int i); //for sf = array[i]
	//void (*mfi)(void* A,int i); //for sf = mf.p[i]
	void **tmp;
	//lerp vs slerp for orientationChaser?
}ftype;

//example for float, but I need SFVec3f versions for position
float *arr3f(float *A, int i){
	//memcpy(T,&A[3*i],3*sizeof(float));
	return &A[3*i];
}
float tmp3f1[6][3];
void *tmp3f [] = {&tmp3f[0],&tmp3f[1],&tmp3f[2],&tmp3f[3],&tmp3f[4],&tmp3f[5]};
ftype ftype_vec3f = {
veccopy3f,
vecadd3f,
vecdif3f,
vecscale3f,
veclength3f,
vecsame3f,
vecsame3f,
arr3f,
tmp3f,
};

struct SFVec3f *sfvec3f_copy(struct SFVec3f* T, struct SFVec3f *A){
	veccopy3f(T->c,A->c);
	return T;
}
struct SFVec3f *sfvec3f_add(struct SFVec3f* T, struct SFVec3f *A, struct SFVec3f *B){
	vecadd3f(T->c,A->c,B->c);
	return T;
}
struct SFVec3f *sfvec3f_dif(struct SFVec3f* T, struct SFVec3f *A, struct SFVec3f *B){
	vecdif3f(T->c,A->c,B->c);
	return T;
}
struct SFVec3f *sfvec3f_scale(struct SFVec3f* T, struct SFVec3f *A, float S){
	vecscale3f(T->c,A->c,S);
	return T;
}
float sfvec3f_dist(struct SFVec3f* A){
	return veclength3f(A->c);
}
int sfvec3f_same(struct SFVec3f *A, struct SFVec3f *B){
	return vecsame3f(A->c,B->c);
}
struct SFVec3f *sfvec3f_arr(struct SFVec3f *A, int i){
	return &A[i];
}
struct SFVec3f sfvec3f_tmps[6];
void *sfvec3f_tmp [] = {&sfvec3f_tmps[0],&sfvec3f_tmps[1],&sfvec3f_tmps[2],&sfvec3f_tmps[3],&sfvec3f_tmps[4],&sfvec3f_tmps[5]};
ftype ftype_sfvec3f = {
sfvec3f_copy,
sfvec3f_add,
sfvec3f_dif,
sfvec3f_scale,
sfvec3f_dist,
sfvec3f_same,
sfvec3f_same,
sfvec3f_arr,
sfvec3f_tmp,
};
#define NEWWAY 1
#ifdef NEWWAY
void chaser_init(struct X3D_PositionChaser *node)
{
	int C;
	chaser_ptrs *p = node->_p;
	ftype *t = node->_t;

	//struct SFVec3f *buffer = (struct SFVec3f*)node->_buffer;
    //node->_destination = node->initialDestination;
	t->copy(p->_destination,p->initialDestination);
    //buffer[0]= node->initialDestination; //initial_destination;
	t->copy(t->arr(p->_buffer,0),p->initialDestination);
    for(C= 1; C<Buffer_length; C++ )
        //buffer[C]= node->initialValue; //initial_value;
		t->copy(t->arr(p->_buffer,C),p->initialValue);
   // node->_previousvalue= node->initialValue; //initial_value;
	t->copy(p->_previousValue,p->initialValue);
    node->_steptime= node->duration / (double) Buffer_length; //cNumSupports;
}
double chaser_UpdateBuffer(struct X3D_PositionChaser *node, double Now)
{
	int C;
    double Frac;
	//struct SFVec3f *buffer = (struct SFVec3f*)node->_buffer;
	chaser_ptrs *p = node->_p;
	ftype *t = node->_t;
	
	Frac = (Now - node->_bufferendtime) / node->_steptime;
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

            //node->_previousvalue= buffer[Buffer_length - NumToShift];
			t->copy(p->_previousValue,t->arr(p->_buffer,Buffer_length - NumToShift));
            for( C= Buffer_length - 1; C>=NumToShift; C-- )
                //buffer[C]= buffer[C - NumToShift];
				t->copy(t->arr(p->_buffer,C),t->arr(p->_buffer,C - NumToShift));
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
				//printf("alf %f ",Alpha);
				// buff[C] = alpha*buff[NumToShift] + (1-alpha)*destination;
				//if(1){
					//float tmp3[3];
					//vecscale3f(tmp1,buffer[NumToShift].c,Alpha);
					t->scale(t->tmp[0],t->arr(p->_buffer,NumToShift),Alpha);
					//vecscale3f(tmp2,node->_destination.c,1.0f - Alpha);
					t->scale(t->tmp[1],p->_destination,1.0f - Alpha);
					//vecadd3f(tmp3,tmp1,tmp2);
					t->add(t->tmp[2],t->tmp[0],t->tmp[1]);
					//veccopy3f(buffer[C].c,tmp3);
					t->copy(t->arr(p->_buffer,C),t->tmp[2]);
				//}else{
				//	vecadd3f(buffer[C].c,vecscale3f(tmp1,buffer[NumToShift].c,Alpha),vecscale3f(tmp2,node->_destination.c,1.0f - Alpha));
				//}
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
            //node->_previousvalue= NumToShift == Buffer_length? buffer[0] : node->_destination;
			if(NumToShift == Buffer_length)
				t->copy(p->_previousValue,t->arr(p->_buffer,0));
			else
				t->copy(p->_previousValue,p->_destination);
            for( C= 0; C<Buffer_length; C++ )
                //buffer[C]= node->_destination;
				t->copy(t->arr(p->_buffer,C),p->_destination);
        }
        node->_bufferendtime+= NumToShift * node->_steptime;
    }
    return Frac;
}
//when a route toNode.toField is PositionChaser.set_destination
//we need to call this function (somehow) much like a script?
//
void chaser_set_destination(struct X3D_PositionChaser *node, double Now)
{
	chaser_ptrs *p = node->_p;
	ftype *t = node->_t;

   // node->_destination= Dest;
	t->copy(p->_destination,p->set_destination);
    // Somehow we assign to Buffer[-1] and wait untill this gets shifted into the real buffer.
    // Would we assign to Buffer[0] instead, we'd have no delay, but this would create a jump in the
    // output because Buffer[0] is associated with a value in the past.
    chaser_UpdateBuffer(node, Now);
}
// This function defines the shape of how the output responds to the input.
// It must accept values for T in the range 0 <= T <= 1.
// In order to create a smooth animation, it should return 0 for T == 0,
// 1 for T == 1 and be sufficient smooth in the range 0 <= T <= 1.

// It should be optimized for speed, in order for high performance. It's
// executed Buffer.length + 1 times each simulation tick.

double chaser_StepResponseCore(double T)
{
    return .5 - .5 * cos(T * PI);
}
double chaser_StepResponse(struct X3D_PositionChaser *node, double t)
{
    if(t < 0.0)
        return 0.0;
    if(t > node->duration)
        return 1.0;
    // When optimizing for speed, the above two if(.) cases can be omitted,
    // as this funciton will not be called for values outside of 0..duration.
    return chaser_StepResponseCore(t / node->duration);
}

void chaser_tick(struct X3D_PositionChaser *node, double Now)
{
	int C;
	double Frac;
    //struct SFVec3f Output;
    //struct SFVec3f DeltaIn;
    //struct SFVec3f DeltaOut;
	void *Output, *DeltaIn, *DeltaOut;
	struct SFVec3f *buffer = (struct SFVec3f*)node->_buffer;
	chaser_ptrs *p = node->_p;
	ftype *t = node->_t;

	Output = t->tmp[3];
	DeltaIn = t->tmp[4];
	DeltaOut = t->tmp[5];
	

	//chaser_CheckInit(node);
    if(!node->_bufferendtime)
    {
        node->_bufferendtime= Now; // first event we received, so we are in the initialization phase.
        //node->value_changed= node->initialValue; //initial_value;
		t->copy(p->value_changed,p->initialValue);
        return;
    }
    Frac= chaser_UpdateBuffer(node, Now);
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

    //Output= node->_previousvalue;
	t->copy(Output,p->_previousValue);
    //DeltaIn= Buffer[Buffer_length - 1].subtract(previousValue);
	//vecdif3f(DeltaIn.c,buffer[Buffer_length - 1].c,node->_previousvalue.c);
	t->dif(DeltaIn,t->arr(p->_buffer,Buffer_length -1),p->_previousValue);

    //DeltaOut= DeltaIn.multiply(StepResponse((Buffer_length - 1 + Frac) * cStepTime));
	//vecscale3f(DeltaOut.c,DeltaIn.c,(float)chaser_StepResponse(node,((double)Buffer_length - 1.0 + Frac) * node->_steptime));
	t->scale(DeltaOut,DeltaIn,(float)chaser_StepResponse(node,((double)Buffer_length - 1.0 + Frac) * node->_steptime));

    //Output= Output.add(DeltaOut);
	//vecadd3f(Output.c,Output.c,DeltaOut.c);
	t->add(Output,Output,DeltaOut);
    for(C= Buffer_length - 2; C>=0; C-- )
    {
        //DeltaIn= Buffer[C].subtract(Buffer[C + 1]);
		//vecdif3f(DeltaIn.c,buffer[C].c,buffer[C+1].c);
		t->dif(DeltaIn,t->arr(p->_buffer,C),t->arr(p->_buffer,C+1));

        //DeltaOut= DeltaIn.multiply(StepResponse((C + Frac) * cStepTime));
		//vecscale3f(DeltaOut.c,DeltaIn.c,(float)chaser_StepResponse(node,((double)C + Frac) * node->_steptime));
		t->scale(DeltaOut,DeltaIn,(float)chaser_StepResponse(node,((double)C + Frac) * node->_steptime));
        //Output= Output.add(DeltaOut);
		//vecadd3f(Output.c,Output.c,DeltaOut.c);
		t->add(Output,Output,DeltaOut);
    }
	//if(!vecsame3f(Output.c,node->value_changed.c)){
	if(!t->same(Output,p->value_changed)){
        t->copy(p->value_changed,Output);
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, value_changed));
	}
}
void chaser_set_value(struct X3D_PositionChaser *node)
{
	chaser_ptrs *p = node->_p;
	ftype *t = node->_t;

    //node->value_changed= opos;
	t->copy(p->value_changed,p->set_value);
	//node->initialValue = opos;
	t->copy(p->initialValue,p->set_value);
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, value_changed));
 	node->isActive = TRUE;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, isActive));
}


void do_PositionChaserTick(void * ptr){
	double Now;
	static double lasttime;
	struct X3D_PositionChaser *node = (struct X3D_PositionChaser *)ptr;
	if(!node) return;
	if(!node->_buffer){
		chaser_ptrs *p = malloc(sizeof(chaser_ptrs));
		node->_buffer = realloc(node->_buffer,Buffer_length * sizeof(struct SFVec3f));
		node->_t = &ftype_sfvec3f;
		node->_p = p;
		p->initialDestination = &node->initialDestination;
		p->initialValue = &node->initialValue;
		p->set_destination = &node->set_destination;
		p->set_value = &node->set_value;
		p->value_changed = &node->value_changed;
		p->_buffer = node->_buffer;
		p->_destination = &node->_destination;
		p->_previousValue = &node->_previousvalue;
		chaser_init(node);
	}
	Now = TickTime();
	if(NODE_NEEDS_COMPILING){
		chaser_ptrs *p = node->_p;
		ftype *t = node->_t;

		node->isActive = TRUE;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, isActive));
		//Q how to tell which set_ was set: set_destination or set_value?
		//if(!vecsame3f(node->set_destination.c,node->_destination.c))
		if(!t->same(p->set_destination,p->_destination))
			chaser_set_destination(node, Now);
		//else if(!vecsame3f(node->set_value.c,node->initialValue.c)) //not sure I have the right idea here
		else if(!t->same(p->set_value,p->initialValue))
			chaser_set_value(node);
		MARK_NODE_COMPILED
	}
	if(node->isActive)
		chaser_tick(node,Now);
}

/*
	//positiondamper

*/
//static int bInitializedD = FALSE;
//static double lastTick = 0.0;
//static int bNeedToTakeFirstInput = TRUE;
//static struct SFVec3f value5 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value4 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value3 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value2 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value1 = {0.0f,0.0f,0.0f};
//static struct SFVec3f input = {0.0f,0.0f,0.0f};
//


void damper_set_value(struct X3D_PositionDamper *node, struct SFVec3f opos);
void damper_Init(struct X3D_PositionDamper *node)
{
    node->_takefirstinput = TRUE;
    damper_set_value(node,node->initialValue);
	node->isActive = TRUE;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
}

float damper_GetDist(struct X3D_PositionDamper *node)
{
	float tmp[3], dist;
	struct SFVec3f *values = (struct SFVec3f *)node->_values;

    //double dist= value1.subtract(node->initialDestination).length();
	dist = veclength3f(vecdif3f(tmp,values[0].c,node->_input.c));
    if(node->order > 1)
    {
        //double dist2= value2.subtract(value1).length();
		float dist2 = veclength3f(vecdif3f(tmp,values[1].c,values[0].c));
        if( dist2 > dist)  dist= dist2;
    }
    if(node->order > 2)
    {
        //double dist3= value3.subtract(value2).length();
		float dist3 = veclength3f(vecdif3f(tmp,values[2].c,values[1].c));
        if( dist3 > dist)  dist= dist3;
    }
    if(node->order > 3)
    {
        //double dist4= value4.subtract(value3).length();
		float dist4 = veclength3f(vecdif3f(tmp,values[3].c,values[2].c));
        if( dist4 > dist)  dist= dist4;
    }
    if(node->order > 4)
    {
        //double dist5= value5.subtract(value4).length();
		float dist5 = veclength3f(vecdif3f(tmp,values[4].c, values[3].c));
        if( dist5 > dist)  dist= dist5;
    }
    return dist;
}
void damper_set_value(struct X3D_PositionDamper *node, struct SFVec3f opos)
{
	struct SFVec3f *values = (struct SFVec3f *)node->_values;
    node->_takefirstinput = FALSE; 
    values[0]= values[1]= values[2]= values[3]= values[4]= opos;
    node->value_changed= opos;
	node->initialValue = opos;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));
 	node->isActive = TRUE;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
}

void damper_set_destination(struct X3D_PositionDamper *node, struct SFVec3f ipos)
{
    if(node->_takefirstinput) 
    {
        node->_takefirstinput = FALSE; 
        damper_set_value(node,ipos);
    }
    if(!vecsame3f(ipos.c,node->_input.c))
    {
        node->_input = ipos;
		node->isActive = TRUE;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
    }
}

struct SFVec3f damper_diftimes(struct SFVec3f a, struct SFVec3f b, double alpha){
	struct SFVec3f ret;
	float tmp[3], tmp2[3];
	//input  .add(value1.subtract(input  ).multiply(alpha))
	if(1){
		vecdif3f(tmp,b.c,a.c);
		vecscale3f(tmp2,tmp,(float)alpha);
		vecadd3f(ret.c,a.c,tmp2);
	}else{
		vecadd3f(ret.c,a.c,vecscale3f(tmp2,vecdif3f(tmp,b.c,a.c),(float)alpha));
	}
	return ret;
}

void tick_positiondamper(struct X3D_PositionDamper *node, double now)
{
	double delta,alpha;
	float dist;
	struct SFVec3f *values = (struct SFVec3f *)node->_values;

    if(!node->_lasttick)
    {
        node->_lasttick= now;
        return;
    }
    delta= now - node->_lasttick;
    node->_lasttick= now;
    alpha= exp(-delta / node->tau);
    if(node->_takefirstinput)  // then don't do any processing.
        return;

    values[0]= node->order > 0 && node->tau != 0.0
               //? input  .add(value1.subtract(input  ).multiply(alpha))
			   ? damper_diftimes(node->_input,values[0],alpha)
               : node->_input;

    values[1]= node->order > 1 && node->tau != 0.0
               //? value1.add(value2.subtract(value1).multiply(alpha))
			   ? damper_diftimes(values[0],values[1],alpha)
               : values[0];

    values[2]= node->order > 2 && node->tau != 0.0
               //? value2.add(value3.subtract(value2).multiply(alpha))
			   ? damper_diftimes(values[1],values[2],alpha)
               : values[1];

    values[3]= node->order > 3 && node->tau != 0.0
               //? value3.add(value4.subtract(value3).multiply(alpha))
			   ? damper_diftimes(values[2],values[3],alpha)
               : values[2];

    values[4]= node->order > 4 && node->tau != 0.0
               //? value4.add(value5.subtract(value4).multiply(alpha))
			   ? damper_diftimes(values[3],values[4],alpha)
               : values[3];

    dist= damper_GetDist(node);

    if(dist < max(node->tolerance,.001f)) //eps)
    {
        values[0]= values[1]= values[2]= values[3]= values[4]= node->_input;
        node->value_changed= node->_input;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));
 	    node->isActive = FALSE;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
        return;
    }
    node->value_changed= values[4];
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));
}
void damper_set_tau(struct X3D_PositionDamper *node, double tau){
    node->_tau = tau;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, tau));
}
void do_PositionDamperTick(void * ptr){
	struct X3D_PositionDamper *node = (struct X3D_PositionDamper *)ptr;
	if(!node)return;
	if(!node->_values){
		node->_values = realloc(node->_values,5 * sizeof(struct SFVec3f));
		//damper_CheckInit(node);
        damper_Init(node);
	}
		
	if(NODE_NEEDS_COMPILING){
		//node->isActive = TRUE;
		if(!vecsame3f(node->set_destination.c,node->_input.c))  //not sure i have the right idea
			damper_set_destination(node, node->set_destination);
		//set_tau 
		if(node->tau != node->_tau)
			damper_set_tau(node,node->tau);
		//set_value
		if(!vecsame3f(node->initialValue.c,node->set_value.c))
			damper_set_value(node,node->set_value);
		MARK_NODE_COMPILED
	}
	if(node->isActive)
		tick_positiondamper(node,TickTime());
}
#else //NEWWAY
void chaser_init(struct X3D_PositionChaser *node)
{
	int C;
	struct SFVec3f *buffer = (struct SFVec3f*)node->_buffer;
    node->_destination = node->initialDestination;
    buffer[0]= node->initialDestination; //initial_destination;
    for(C= 1; C<Buffer_length; C++ )
        buffer[C]= node->initialValue; //initial_value;
    node->_previousvalue= node->initialValue; //initial_value;
    node->_steptime= node->duration / (double) Buffer_length; //cNumSupports;
}
double chaser_UpdateBuffer(struct X3D_PositionChaser *node, double Now)
{
	int C;
    double Frac;
	struct SFVec3f *buffer = (struct SFVec3f*)node->_buffer;
	
	Frac = (Now - node->_bufferendtime) / node->_steptime;
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

            node->_previousvalue= buffer[Buffer_length - NumToShift];
            for( C= Buffer_length - 1; C>=NumToShift; C-- )
                buffer[C]= buffer[C - NumToShift];
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
				//printf("alf %f ",Alpha);
				// buff[C] = alpha*buff[NumToShift] + (1-alpha)*destination;
				if(1){
					float tmp3[3];
					vecscale3f(tmp1,buffer[NumToShift].c,Alpha);
					vecscale3f(tmp2,node->_destination.c,1.0f - Alpha);
					vecadd3f(tmp3,tmp1,tmp2);
					veccopy3f(buffer[C].c,tmp3);
				}else{
					vecadd3f(buffer[C].c,vecscale3f(tmp1,buffer[NumToShift].c,Alpha),vecscale3f(tmp2,node->_destination.c,1.0f - Alpha));
				}
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
            node->_previousvalue= NumToShift == Buffer_length? buffer[0] : node->_destination;

            for( C= 0; C<Buffer_length; C++ )
                buffer[C]= node->_destination;
        }
        node->_bufferendtime+= NumToShift * node->_steptime;
    }
    return Frac;
}
//when a route toNode.toField is PositionChaser.set_destination
//we need to call this function (somehow) much like a script?
//
void chaser_set_destination(struct X3D_PositionChaser *node, struct SFVec3f Dest, double Now)
{
    node->_destination= Dest;
    // Somehow we assign to Buffer[-1] and wait untill this gets shifted into the real buffer.
    // Would we assign to Buffer[0] instead, we'd have no delay, but this would create a jump in the
    // output because Buffer[0] is associated with a value in the past.
    chaser_UpdateBuffer(node, Now);
}
// This function defines the shape of how the output responds to the input.
// It must accept values for T in the range 0 <= T <= 1.
// In order to create a smooth animation, it should return 0 for T == 0,
// 1 for T == 1 and be sufficient smooth in the range 0 <= T <= 1.

// It should be optimized for speed, in order for high performance. It's
// executed Buffer.length + 1 times each simulation tick.

double chaser_StepResponseCore(double T)
{
    return .5 - .5 * cos(T * PI);
}
double chaser_StepResponse(struct X3D_PositionChaser *node, double t)
{
    if(t < 0.0)
        return 0.0;
    if(t > node->duration)
        return 1.0;
    // When optimizing for speed, the above two if(.) cases can be omitted,
    // as this funciton will not be called for values outside of 0..duration.
    return chaser_StepResponseCore(t / node->duration);
}

void chaser_tick(struct X3D_PositionChaser *node, double Now)
{
	int C;
	double Frac;
    struct SFVec3f Output;
    struct SFVec3f DeltaIn;
    struct SFVec3f DeltaOut;
	struct SFVec3f *buffer = (struct SFVec3f*)node->_buffer;

	//chaser_CheckInit(node);
    if(!node->_bufferendtime)
    {
        node->_bufferendtime= Now; // first event we received, so we are in the initialization phase.
        node->value_changed= node->initialValue; //initial_value;
        return;
    }
    Frac= chaser_UpdateBuffer(node, Now);
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

    Output= node->_previousvalue;
    //DeltaIn= Buffer[Buffer_length - 1].subtract(previousValue);
	vecdif3f(DeltaIn.c,buffer[Buffer_length - 1].c,node->_previousvalue.c);

    //DeltaOut= DeltaIn.multiply(StepResponse((Buffer_length - 1 + Frac) * cStepTime));
	vecscale3f(DeltaOut.c,DeltaIn.c,(float)chaser_StepResponse(node,((double)Buffer_length - 1.0 + Frac) * node->_steptime));
    //Output= Output.add(DeltaOut);
	vecadd3f(Output.c,Output.c,DeltaOut.c);
    for(C= Buffer_length - 2; C>=0; C-- )
    {
        //DeltaIn= Buffer[C].subtract(Buffer[C + 1]);
		vecdif3f(DeltaIn.c,buffer[C].c,buffer[C+1].c);
        //DeltaOut= DeltaIn.multiply(StepResponse((C + Frac) * cStepTime));
		vecscale3f(DeltaOut.c,DeltaIn.c,(float)chaser_StepResponse(node,((double)C + Frac) * node->_steptime));
        //Output= Output.add(DeltaOut);
		vecadd3f(Output.c,Output.c,DeltaOut.c);
    }
	if(!vecsame3f(Output.c,node->value_changed.c)){
        node->value_changed= Output;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, value_changed));
	}
}
void chaser_set_value(struct X3D_PositionChaser *node, struct SFVec3f opos)
{
    node->value_changed= opos;
	node->initialValue = opos;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, value_changed));
 	node->isActive = TRUE;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, isActive));
}


void do_PositionChaserTick(void * ptr){
	double Now;
	static double lasttime;
	struct X3D_PositionChaser *node = (struct X3D_PositionChaser *)ptr;
	if(!node) return;
	if(!node->_buffer){
		node->_buffer = realloc(node->_buffer,Buffer_length * sizeof(struct SFVec3f));
		chaser_init(node);
	}
	Now = TickTime();
	if(NODE_NEEDS_COMPILING){
		node->isActive = TRUE;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser, isActive));
		//Q how to tell which set_ was set: set_destination or set_value?
		if(!vecsame3f(node->set_destination.c,node->_destination.c))
			chaser_set_destination(node, node->set_destination,Now);
		else if(!vecsame3f(node->set_value.c,node->initialValue.c)) //not sure I have the right idea here
			chaser_set_value(node,node->set_value);
		MARK_NODE_COMPILED
	}
	if(node->isActive)
		chaser_tick(node,Now);
}

/*
	//positiondamper

*/
//static int bInitializedD = FALSE;
//static double lastTick = 0.0;
//static int bNeedToTakeFirstInput = TRUE;
//static struct SFVec3f value5 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value4 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value3 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value2 = {0.0f,0.0f,0.0f};
//static struct SFVec3f value1 = {0.0f,0.0f,0.0f};
//static struct SFVec3f input = {0.0f,0.0f,0.0f};
//


void damper_set_value(struct X3D_PositionDamper *node, struct SFVec3f opos);
void damper_Init(struct X3D_PositionDamper *node)
{
    node->_takefirstinput = TRUE;
    damper_set_value(node,node->initialValue);
	node->isActive = TRUE;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
}

float damper_GetDist(struct X3D_PositionDamper *node)
{
	float tmp[3], dist;
	struct SFVec3f *values = (struct SFVec3f *)node->_values;

    //double dist= value1.subtract(node->initialDestination).length();
	dist = veclength3f(vecdif3f(tmp,values[0].c,node->_input.c));
    if(node->order > 1)
    {
        //double dist2= value2.subtract(value1).length();
		float dist2 = veclength3f(vecdif3f(tmp,values[1].c,values[0].c));
        if( dist2 > dist)  dist= dist2;
    }
    if(node->order > 2)
    {
        //double dist3= value3.subtract(value2).length();
		float dist3 = veclength3f(vecdif3f(tmp,values[2].c,values[1].c));
        if( dist3 > dist)  dist= dist3;
    }
    if(node->order > 3)
    {
        //double dist4= value4.subtract(value3).length();
		float dist4 = veclength3f(vecdif3f(tmp,values[3].c,values[2].c));
        if( dist4 > dist)  dist= dist4;
    }
    if(node->order > 4)
    {
        //double dist5= value5.subtract(value4).length();
		float dist5 = veclength3f(vecdif3f(tmp,values[4].c, values[3].c));
        if( dist5 > dist)  dist= dist5;
    }
    return dist;
}
void damper_set_value(struct X3D_PositionDamper *node, struct SFVec3f opos)
{
	struct SFVec3f *values = (struct SFVec3f *)node->_values;
    node->_takefirstinput = FALSE; 
    values[0]= values[1]= values[2]= values[3]= values[4]= opos;
    node->value_changed= opos;
	node->initialValue = opos;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));
 	node->isActive = TRUE;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
}

void damper_set_destination(struct X3D_PositionDamper *node, struct SFVec3f ipos)
{
    if(node->_takefirstinput) 
    {
        node->_takefirstinput = FALSE; 
        damper_set_value(node,ipos);
    }
    if(!vecsame3f(ipos.c,node->_input.c))
    {
        node->_input = ipos;
		node->isActive = TRUE;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
    }
}

struct SFVec3f damper_diftimes(struct SFVec3f a, struct SFVec3f b, double alpha){
	struct SFVec3f ret;
	float tmp[3], tmp2[3];
	//input  .add(value1.subtract(input  ).multiply(alpha))
	if(1){
		vecdif3f(tmp,b.c,a.c);
		vecscale3f(tmp2,tmp,(float)alpha);
		vecadd3f(ret.c,a.c,tmp2);
	}else{
		vecadd3f(ret.c,a.c,vecscale3f(tmp2,vecdif3f(tmp,b.c,a.c),(float)alpha));
	}
	return ret;
}

void tick_positiondamper(struct X3D_PositionDamper *node, double now)
{
	double delta,alpha;
	float dist;
	struct SFVec3f *values = (struct SFVec3f *)node->_values;

    if(!node->_lasttick)
    {
        node->_lasttick= now;
        return;
    }
    delta= now - node->_lasttick;
    node->_lasttick= now;
    alpha= exp(-delta / node->tau);
    if(node->_takefirstinput)  // then don't do any processing.
        return;

    values[0]= node->order > 0 && node->tau != 0.0
               //? input  .add(value1.subtract(input  ).multiply(alpha))
			   ? damper_diftimes(node->_input,values[0],alpha)
               : node->_input;

    values[1]= node->order > 1 && node->tau != 0.0
               //? value1.add(value2.subtract(value1).multiply(alpha))
			   ? damper_diftimes(values[0],values[1],alpha)
               : values[0];

    values[2]= node->order > 2 && node->tau != 0.0
               //? value2.add(value3.subtract(value2).multiply(alpha))
			   ? damper_diftimes(values[1],values[2],alpha)
               : values[1];

    values[3]= node->order > 3 && node->tau != 0.0
               //? value3.add(value4.subtract(value3).multiply(alpha))
			   ? damper_diftimes(values[2],values[3],alpha)
               : values[2];

    values[4]= node->order > 4 && node->tau != 0.0
               //? value4.add(value5.subtract(value4).multiply(alpha))
			   ? damper_diftimes(values[3],values[4],alpha)
               : values[3];

    dist= damper_GetDist(node);

    if(dist < max(node->tolerance,.001f)) //eps)
    {
        values[0]= values[1]= values[2]= values[3]= values[4]= node->_input;
        node->value_changed= node->_input;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));
 	    node->isActive = FALSE;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, isActive));
        return;
    }
    node->value_changed= values[4];
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, value_changed));
}
void damper_set_tau(struct X3D_PositionDamper *node, double tau){
    node->_tau = tau;
	MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper, tau));
}
void do_PositionDamperTick(void * ptr){
	struct X3D_PositionDamper *node = (struct X3D_PositionDamper *)ptr;
	if(!node)return;
	if(!node->_values){
		node->_values = realloc(node->_values,5 * sizeof(struct SFVec3f));
		//damper_CheckInit(node);
        damper_Init(node);
	}
		
	if(NODE_NEEDS_COMPILING){
		//node->isActive = TRUE;
		if(!vecsame3f(node->set_destination.c,node->_input.c))  //not sure i have the right idea
			damper_set_destination(node, node->set_destination);
		//set_tau 
		if(node->tau != node->_tau)
			damper_set_tau(node,node->tau);
		//set_value
		if(!vecsame3f(node->initialValue.c,node->set_value.c))
			damper_set_value(node,node->set_value);
		MARK_NODE_COMPILED
	}
	if(node->isActive)
		tick_positiondamper(node,TickTime());
}
#endif //NEWWAY

void do_PositionChaser2DTick(void * ptr){
	struct X3D_PositionChaser2D *node = (struct X3D_PositionChaser2D *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		veccopy2f(node->value_changed.c, node->set_destination.c);
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionChaser2D, value_changed));
		MARK_NODE_COMPILED
	}

}
void do_PositionDamper2DTick(void * ptr){
	struct X3D_PositionDamper2D *node = (struct X3D_PositionDamper2D *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		veccopy2f(node->value_changed.c, node->set_destination.c);
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_PositionDamper2D, value_changed));
		MARK_NODE_COMPILED
	}

}

void do_ScalarChaserTick(void * ptr){
	struct X3D_ScalarChaser *node = (struct X3D_ScalarChaser *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		node->value_changed = node->set_destination;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_ScalarChaser, value_changed));
		MARK_NODE_COMPILED
	}
}
void do_ScalarDamperTick(void * ptr){
	struct X3D_ScalarDamper *node = (struct X3D_ScalarDamper *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		node->value_changed = node->set_destination;
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_ScalarDamper, value_changed));
		MARK_NODE_COMPILED
	}
}

void do_TexCoordChaser2DTick(void * ptr){
	struct X3D_TexCoordChaser2D *node = (struct X3D_TexCoordChaser2D *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		int n;
		n = node->set_destination.n;
		node->value_changed.n = n;
		node->value_changed.p = realloc(node->value_changed.p,n * sizeof(struct SFVec2f));
		memcpy(node->value_changed.p,node->set_destination.p,n*sizeof(struct SFVec2f));
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_TexCoordChaser2D, value_changed));
		MARK_NODE_COMPILED
	}
}
void do_TexCoordDamper2DTick(void * ptr){
	struct X3D_TexCoordDamper2D *node = (struct X3D_TexCoordDamper2D *)ptr;
	if(!node)return;
	if(NODE_NEEDS_COMPILING){
		//default action copy input to output when not implemented
		int n;
		n = node->set_destination.n;
		node->value_changed.n = n;
		node->value_changed.p = realloc(node->value_changed.p,n * sizeof(struct SFVec2f));
		memcpy(node->value_changed.p,node->set_destination.p,n*sizeof(struct SFVec2f));
		MARK_EVENT ((struct X3D_Node*)node, offsetof(struct X3D_TexCoordDamper2D, value_changed));
		MARK_NODE_COMPILED
	}
}

//============================


