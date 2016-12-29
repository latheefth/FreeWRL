
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


/*******************************************************************

	X3D Interpolation Component

*********************************************************************/


#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>

#include "../vrml_parser/Structs.h"
#include "../input/InputFunctions.h"
#include "../opengl/LoadTextures.h"        /* for finding a texture url in a multi url */


#include "../main/headers.h"
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/RenderFuncs.h"

#include "../x3d_parser/Bindable.h"
#include "../scenegraph/LinearAlgebra.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/sounds.h"
#include "../vrml_parser/CRoutes.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"            /* for finding a texture url in a multi url */

#include "../input/SensInterps.h"

//defined in Component_RigidBodyPhysics
int NNC0(struct X3D_Node* node); 
void MNC0(struct X3D_Node* node);
void MNX0(struct X3D_Node* node);
#define NNC(A) NNC0(X3D_NODE(A))  //node needs compiling
#define MNC(A) MNC0(X3D_NODE(A))  //mark node compiled
#define MNX(A) MNX0(X3D_NODE(A))  //mark node changed
#define PPX(A) getTypeNode(X3D_NODE(A)) //possible proto expansion

// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html
// see also SenseInterps.c for other interpolator componenent implementations

void do_EaseInEaseOut(void *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#EaseInEaseOut

	/* ScalarInterpolator - store final value in px->value_changed */
	struct X3D_EaseInEaseOut *px;
	int kin, kvin;
	float *kVs;
	int counter;

	if (!node) return;
	px = (struct X3D_EaseInEaseOut *) node;
	kin = px->key.n;

	MARK_EVENT (node, offsetof (struct X3D_EaseInEaseOut, modifiedFraction_changed));

	float fraction = min(px->set_fraction,px->key.p[kin-1]);
	fraction = max(px->set_fraction,px->key.p[0]);
	/* have to go through and find the key before */
	int ispan =find_key(kin,fraction,px->key.p);
	float u = (fraction - px->key.p[ispan-1]) / (px->key.p[ispan] - px->key.p[ispan-1]);
	float eout = px->easeInEaseOut.p[ispan].c[1]; //y
	float ein  = px->easeInEaseOut.p[ispan+1].c[0]; //x
	float S = eout + ein;

	if(S < 0.0f){
		px->modifiedFraction_changed = u;
	}else{
		if(S > 1.0f){
			ein /= S;
			eout /= S;
		}
		float t = 1.0f / (2.0f - eout - ein);
		if(u < eout){
			px->modifiedFraction_changed = (t/eout) * u*u;
		}else if(u < 1.0f - ein){
			px->modifiedFraction_changed = (t*(2*u - eout));
		}else{
			px->modifiedFraction_changed = 1.0f - (t*(1.0f - u)*(1.0f - u))/ein;
		}
	}
		
	
}

/*
	Splines via Hermite
	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#HermiteSplineInterpolation
	https://en.wikipedia.org/wiki/Cubic_Hermite_spline
	- you give it a slope and position at each end, and a 0-1 t
	- you can compute for xyz by doing 3 separate scalar interpolations one for each coordinate
	- or to save FLOPS (floating point ops) you can do a template algo like we did for Followers

	if linear interp looks like this:
	fraction s = fraction_interp(t,t0,t1);
	answer = linear_interp(s,key0,val0,key1,val1)
	
	Then spline interp would look like this:
	fraction s = fraction_interp(t,t0,t1);
	answer = spline_interp(s,key0,val0,vel0,key1,val1,vel1)  
	- where vel is velocity or more generally slope/first-derivitive of value at key. 
	-- and first derivitive would be with respect to key ie d(Value)/d(key) evaluated at key
	-- and according to specs you calculate defaults if user doesn't supply
	- and in general {answer, val*, vel*} are vectors or types ie SFVec3f, SFRotation, SFfloat

*/
void spline_interp(int dim, float *result, float s, float *val0, float *val1, float *T00, float *T11){
	// dim - dimension of val and vel ie 3 for xyz
	// interpolates one value, given the span values
	float S[4], *C[4], SH[4];
	static float H[16] = {
	2.0f, -2.0f, 1.0f, 1.0f,
	-3.0f,  3.0f,-2.0f,-1.0f,
	0.0f,  0.0f, 1.0f, 0.0f,
	1.0f,  0.0f, 0.0f, 0.0f};

	S[0] = s*s*s;
	S[1] = s*s;
	S[2] = s;
	S[3] = 1.0f;
	vecmultmat4f(SH,S,H);

	C[0] = val0; //vi
	C[1] = val1; //vi+1
	C[2] = T00; //T0i
	C[3] = T11; //T1i+1
		
	for(int i=0;i<dim;i++){
		result[i] = 0.0f;
		for(int j=0;j<4;j++){
			result[i] += SH[j]*C[j][i];
		}
	}
}

//for xyz dim =3, for xy dim=2 for scalar dim=1
void compute_spline_velocity_Ti(int dim, int normalize, int nval, float *val, int nvel, float* vel, float *Ti ){
	//please malloc Ti = malloc(nval*dim*sizeof(float)) before calling
	if(nvel && vel && nvel == nval){
		if(!normalize){
			//If the velocity vector is specified, and the normalizeVelocity flag has value FALSE, 
			//the velocity at the key is set to the corresponding value of the keyVelocity field:
			//Ti = keyVelocity[ i ]
			memcpy(Ti,vel,dim*nval*sizeof(float));
		}else{
			//If the velocity vector is specified, and the normalizeVelocity flag is TRUE,
			// the velocity at the key is set using the corresponding value of the keyVelocity field:
			//Ti = keyVelocity[i] × ( Dtot / |keyVelocity[i]| )

			//Dtot = SUM{i=0, i < n-1}(|vi - vi+1|)
			float Dtot = 0.0f;
			for(int i=0;i<nval-1;i++){
				float Di = 0.0f;
				for(int j=0;j<dim;j++){
					Di += (val[i+1] - val[i])*(val[i+1] - val[i]); //euclidean distance d= sqrt(x**2 + y**2)
				}
				Dtot += sqrt(Di);
			}
			for(int i=0;i<nval;i++){
				float veli = 0.0f;
				for(int j=0;j<dim;j++)
					veli = veli + (vel[i*dim + j]*vel[i*dim + j]); //euclidean distance d= sqrt(x**2 + y**2)
				veli = sqrt(veli);
				for(int j=0;j<dim;j++){
					if(veli != 0.0f)
						Ti[i*dim + j] = Dtot * vel[i*dim + j] / veli;
					else
						Ti[i*dim + j] = 0.0f;
				}
			}
		}
	}else{
		//If the velocity vector is not specified, (or just start & end,) it is calculated as follows:
		//Ti = (vi+1 - vi-1) / 2
		if(nvel == 2){
			for(int j=0;j<dim;j++){
				Ti[       0*dim + j] = vel[0*dim + j];
				Ti[(nval-1)*dim + j] = vel[1*dim + j];
			}

		}else{
			for(int j=0;j<dim;j++){
				Ti[       0*dim + j] = 0.0f;
				Ti[(nval-1)*dim + j] = 0.0f;
			}
		}
		//skip start and end vels here
		for(int i=1;i<nval-1;i++)
			for(int j=0;j<dim;j++)
				Ti[i*dim + j] = (val[(i+1)*dim +j] - val[(i-1)*dim +j]) * .5f;

	}
}
int iwrap(int i, int istart, int iend){
	//if they don't duplicate - the last point != first point - then iend = n
	//if they duplicate last point == first point, then iend = n-1
	// normally istart = 0, iend = n
	// 6 = iwrap(-1,0,7)
	// 0 = iwrap(7,0,7)
	int iret = i;
	if(iret < istart) iret = iend - (istart - iret);
	if(iret >= iend ) iret = istart + (iret - iend);
	return iret;
}
void spline_velocity_adjust_for_keyspan(int dim, int closed, int nval, float *key, float *Ti, float* T0, float *T1){
	//before calling please malloc T0 and T1 = malloc(nval * dim * sizeof(float))
	float Fp, Fm;
	int istart, iend;
	istart = 1;
	iend = nval-1;
	if(closed){
		istart = 0;
		iend = nval;
	}else{
		//take first and last values from Ti which were either 0 or start/end
		int l = nval-1;
		for(int j=0;j<dim;j++){
			T1[0*dim +j] = T0[0*dim +j] = Ti[0*dim +j];
			T1[l*dim +j] = T0[l*dim +j] = Ti[l*dim +j];
		}
	}
	for(int i=istart;i<iend;i++){
		int ip, im;
		ip = iwrap(i+1,0,nval);
		im = iwrap(i-1,0,nval);
		Fm = 2.0f*(key[ip] - key[i])/(key[ip]-key[im]);
		Fp = 2.0f*(key[i] - key[im])/(key[ip]-key[im]);
		for(int j=0;j<dim;j++){
			T0[i*dim +j] = Fp*Ti[i*dim +j];
			T1[i*dim +j] = Fm*Ti[i*dim +j];
		}
	}
}


void do_SplinePositionInterpolator(void *node){
	//	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#SplinePositionInterpolator
	int dim;
	struct X3D_SplinePositionInterpolator* px = (struct X3D_SplinePositionInterpolator*)node;
	dim = 3;
	if(NNC(px)){
		
		//void compute_spline_velocity_Ti(int dim, int normalize, int nval, float *val, int nvel, float* vel, float *Ti )
		int n = px->key.n;
		float *Ti = MALLOC(float*,n*dim*sizeof(float));
		compute_spline_velocity_Ti(dim,px->normalizeVelocity,n,(float*)px->keyValue.p,
			px->keyVelocity.n,(float*)px->keyVelocity.p,Ti);
		float *T0 = MALLOC(float*,n*dim*sizeof(float));
		float *T1 = MALLOC(float*,n*dim*sizeof(float));
		//spline_velocity_adjust_for_keyspan(int dim, int closed, int nval, float *key, float *Ti, float* T0, float *T1)
		spline_velocity_adjust_for_keyspan(dim,px->closed,n,px->key.p,Ti,T0,T1);
		FREE_IF_NZ(px->_T0.p);
		FREE_IF_NZ(px->_T1.p);
		px->_T0.p = (struct SFVec3f*)T0;
		px->_T0.n = n;
		px->_T1.p = (struct SFVec3f*)T1;
		px->_T1.n = n;
		FREE_IF_NZ(Ti);

		MNC(px);
	}
	//void spline_interp(int dim, float *result, float s, float *val0, float *val1, float *T00, float *T11)
	int kin, kvin;
	int counter;

	if (!node) return;
	kin = px->key.n;
	kvin = px->keyValue.n;

	MARK_EVENT (node, offsetof (struct X3D_SplinePositionInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		vecset3f(px->value_changed.c,0.0f,0.0f,0.0f);
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */

	#ifdef SEVERBOSE
		printf ("ScalarInterpolator, kin %d kvin %d, vc %f\n",kin,kvin,px->value_changed);
	#endif

	/* set_fraction less than or greater than keys */
	float fraction = min(px->set_fraction,px->key.p[kin-1]);
	fraction = max(px->set_fraction,px->key.p[0]);
	/* have to go through and find the key before */
	int ispan =find_key(kin,fraction,px->key.p);

	// INTERPOLATION FUNCTION - change this from linear to spline
	// fraction s = fraction_interp(t,t0,t1);
	// answer = linear_interp(s,key0,val0,key1,val1)
	// answer = spline_interp(s,key0,val0,vel0,key1,val1,vel1)  
	//  where vel is velocity or more generally slope/first-derivitive of value at key. 
	//  First derivitive would be with respect to key ie d(Value)/d(key) evaluated at key
	// in general answer, val*, vel* are vectors or types.

	spline_interp(dim, px->value_changed.c, fraction,
		px->keyValue.p[ispan].c, px->keyValue.p[ispan+1].c, 
		px->_T0.p[ispan].c, px->_T1.p[ispan].c);
}
void do_SplinePositionInterpolator2D(void *node){
	int dim;
	struct X3D_SplinePositionInterpolator2D* px = (struct X3D_SplinePositionInterpolator2D*)node;
	dim = 2;
	if(NNC(px)){
		
		//void compute_spline_velocity_Ti(int dim, int normalize, int nval, float *val, int nvel, float* vel, float *Ti )
		int n = px->key.n;
		float *Ti = MALLOC(float*,n*dim*sizeof(float));
		compute_spline_velocity_Ti(dim,px->normalizeVelocity,n,(float*)px->keyValue.p,
			px->keyVelocity.n,(float*)px->keyVelocity.p,Ti);
		float *T0 = MALLOC(float*,n*dim*sizeof(float));
		float *T1 = MALLOC(float*,n*dim*sizeof(float));
		//spline_velocity_adjust_for_keyspan(int dim, int closed, int nval, float *key, float *Ti, float* T0, float *T1)
		spline_velocity_adjust_for_keyspan(dim,px->closed,n,px->key.p,Ti,T0,T1);
		FREE_IF_NZ(px->_T0.p);
		FREE_IF_NZ(px->_T1.p);
		px->_T0.p = (struct SFVec2f*)T0;
		px->_T0.n = n;
		px->_T1.p = (struct SFVec2f*)T1;
		px->_T1.n = n;
		FREE_IF_NZ(Ti);

		MNC(px);
	}
	//void spline_interp(int dim, float *result, float s, float *val0, float *val1, float *T00, float *T11)
	int kin, kvin;
	int counter;

	if (!node) return;
	kin = px->key.n;
	kvin = px->keyValue.n;

	MARK_EVENT (node, offsetof (struct X3D_SplinePositionInterpolator2D, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		vecset2f(px->value_changed.c,0.0f,0.0f);
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */

	#ifdef SEVERBOSE
		printf ("ScalarInterpolator, kin %d kvin %d, vc %f\n",kin,kvin,px->value_changed);
	#endif

	/* set_fraction less than or greater than keys */
	float fraction = min(px->set_fraction,px->key.p[kin-1]);
	fraction = max(px->set_fraction,px->key.p[0]);
	/* have to go through and find the key before */
	int ispan =find_key(kin,fraction,px->key.p);

	// INTERPOLATION FUNCTION - change this from linear to spline
	// fraction s = fraction_interp(t,t0,t1);
	// answer = linear_interp(s,key0,val0,key1,val1)
	// answer = spline_interp(s,key0,val0,vel0,key1,val1,vel1)  
	//  where vel is velocity or more generally slope/first-derivitive of value at key. 
	//  First derivitive would be with respect to key ie d(Value)/d(key) evaluated at key
	// in general answer, val*, vel* are vectors or types.

	spline_interp(dim, px->value_changed.c, fraction,
		px->keyValue.p[ispan].c, px->keyValue.p[ispan+1].c, 
		px->_T0.p[ispan].c, px->_T1.p[ispan].c);

}
void do_SplineScalarInterpolator(void *node){
	// SplineScalarInterpolator - store final value in px->value_changed 
	//  - body of function copied from ScalarInterpolator and node cast to SplineScalarInterpolator
	int dim;
	struct X3D_SplineScalarInterpolator* px = (struct X3D_SplineScalarInterpolator*)node;
	dim = 1;
	if(NNC(px)){
		
		//void compute_spline_velocity_Ti(int dim, int normalize, int nval, float *val, int nvel, float* vel, float *Ti )
		int n = px->key.n;
		float *Ti = MALLOC(float*,n*dim*sizeof(float));
		compute_spline_velocity_Ti(dim,px->normalizeVelocity,n,(float*)px->keyValue.p,
			px->keyVelocity.n,(float*)px->keyVelocity.p,Ti);
		float *T0 = MALLOC(float*,n*dim*sizeof(float));
		float *T1 = MALLOC(float*,n*dim*sizeof(float));
		//spline_velocity_adjust_for_keyspan(int dim, int closed, int nval, float *key, float *Ti, float* T0, float *T1)
		spline_velocity_adjust_for_keyspan(1,px->closed,n,px->key.p,Ti,T0,T1);
		FREE_IF_NZ(px->_T0.p);
		FREE_IF_NZ(px->_T1.p);
		px->_T0.p = T0;
		px->_T0.n = n;
		px->_T1.p = T1;
		px->_T1.n = n;
		FREE_IF_NZ(Ti);

		MNC(px);
	}
	//void spline_interp(int dim, float *result, float s, float *val0, float *val1, float *T00, float *T11)
	int kin, kvin;
	int counter;

	if (!node) return;
	kin = px->key.n;
	kvin = px->keyValue.n;

	MARK_EVENT (node, offsetof (struct X3D_SplineScalarInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed = 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */

	#ifdef SEVERBOSE
		printf ("ScalarInterpolator, kin %d kvin %d, vc %f\n",kin,kvin,px->value_changed);
	#endif

	/* set_fraction less than or greater than keys */
	float fraction = min(px->set_fraction,px->key.p[kin-1]);
	fraction = max(px->set_fraction,px->key.p[0]);
	/* have to go through and find the key before */
	int ispan =find_key(kin,fraction,px->key.p);

	// INTERPOLATION FUNCTION - change this from linear to spline
	// fraction s = fraction_interp(t,t0,t1);
	// answer = linear_interp(s,key0,val0,key1,val1)
	// answer = spline_interp(s,key0,val0,vel0,key1,val1,vel1)  
	//  where vel is velocity or more generally slope/first-derivitive of value at key. 
	//  First derivitive would be with respect to key ie d(Value)/d(key) evaluated at key
	// in general answer, val*, vel* are vectors or types.

	spline_interp(dim, (float*)&px->value_changed, fraction,
		&px->keyValue.p[ispan], &px->keyValue.p[ispan+1], 
		&px->_T0.p[ispan], &px->_T1.p[ispan]);

}


// START MIT LIC >>>>

double *quaternion2double(double *xyzw, Quaternion *q){
	xyzw[0] = q->x;
	xyzw[1] = q->y;
	xyzw[2] = q->z;
	xyzw[3] = q->w;
	return xyzw;
}
Quaternion *double2quaternion(Quaternion *q, double* xyzw){
	q->x = xyzw[0];
	q->y = xyzw[1];
	q->z = xyzw[2];
	q->w = xyzw[3];
	return q;
}
void quaternion_addition(Quaternion *ret, const Quaternion *q1, const Quaternion *q2){
	//the quaternion_add in Quanternions.c seems too complicated. 
	// supposed to be q+q' = [s+s',v+v']
	double xyzw1[4],xyzw2[4],xyzw[4];
	quaternion2double(xyzw1,q1);
	quaternion2double(xyzw2,q2);
	vecaddd(xyzw,xyzw1,xyzw2);
	xyzw[3] = xyzw1[3] + xyzw2[3];
	double2quaternion(ret,xyzw);
}
void quaternion_subtraction(Quaternion *ret, const Quaternion *q1, const Quaternion *q2){
	// supposed to be q-q' = [s-s',v-v']
	double xyzw1[4],xyzw2[4],xyzw[4];
	quaternion2double(xyzw1,q1);
	quaternion2double(xyzw2,q2);
	vecdifd(xyzw,xyzw1,xyzw2);
	xyzw[3] = xyzw1[3] - xyzw2[3];
	double2quaternion(ret,xyzw);
}
double clampd(double val, double low, double hi){
	double ret = val;
	ret = min(ret,hi);
	ret = max(ret,low);
	return ret;
}
double quaternion_dot(Quaternion *q1, Quaternion *q2){
	double xyzw1[4], xyzw2[4], dot;
	quaternion2double(xyzw1,q1);
	quaternion2double(xyzw2,q1);
	dot = 0.0;
	for(int i=0;i<4;i++)
		dot += xyzw1[i]*xyzw2[i];
	return dot; 
}
void quaternion_negate(Quaternion *q){
	double xyzw[4];
	quaternion2double(xyzw,q);
	for(int i=0;i<4;i++)
		xyzw[i] = -xyzw[i];
	double2quaternion(q,xyzw);
}
void quaternion_log(Quaternion *ret, Quaternion *q){
	double angle, angle_over_sine, xyzw[4];

	quaternion2double(xyzw,q);
	angle = acos( clampd(xyzw[3],-1.0,1.0));
	angle_over_sine = 0.0;
	if(angle != 0.0) angle_over_sine = angle/sin(angle);
	vecscaled(xyzw,xyzw,angle_over_sine);
	xyzw[3]=0.0;
	double2quaternion(ret,xyzw);
}
void quaternion_exp(Quaternion *ret, Quaternion *q){
	double angle, xyzw[4], sine, sine_over_angle;

	quaternion2double(xyzw,q);
	angle = veclengthd(xyzw);
	sine_over_angle = 0.0;
	if(angle != 0.0) sine_over_angle = sin(angle) / angle;
	vecscaled(xyzw,xyzw,sine_over_angle);
	xyzw[3] = cos(angle);
	double2quaternion(ret,xyzw);
}
void compute_si(Quaternion *si,Quaternion *qi, Quaternion *qip1, Quaternion *qim1){
	//si is like the (quaternion-space) slope at point qi, or like a bezier tangent control point
	//and is supposed to be the average from either side of (quaternion-space) point qi
	//or more precisely, the 2 slopes / 1st derivitives are set equal
	// http://web.mit.edu/2.998/www/QuaternionReport1.pdf  
	//		si = qi * exp( - (log(inv(qi)*qi+1) + log(inv(qi)*qi-1)) / 4 )
	//		and qi+1 means q[i+1] and qi-1 means q[i-1] ie that's an indexer, not a quat + scalar
	//	p. 15 shows log(q) if q=[cost,sint*v] then 
	//			log(q) = [0, t*v] (non-unit-sphere in quaternion space, not in H1)
	//			exp(q) = [cost, sint*v] (puts cos(t) back in scalar part, inverting log)
	// http://www.cs.ucr.edu/~vbz/resources/quatut.pdf
	// p.10 "the logarithm of a unit quaternion q = [vˆ sin O, cos O] is just vˆO, a pure vector of length O."

	Quaternion qiinv, qiinv_qip1, qiinv_qim1,qadded,qexp;
	double xyzw[4],sine,cosine,angle, angle_over_sine;

	quaternion_inverse(&qiinv,qi);
	quaternion_multiply(&qiinv_qip1,&qiinv,qip1);

	quaternion_log(&qiinv_qip1,&qiinv_qip1);
	quaternion_multiply(&qiinv_qim1,&qiinv,qim1);
	quaternion_log(&qiinv_qim1,&qiinv_qim1);

	quaternion_addition(&qadded,&qiinv_qip1,&qiinv_qim1); //don't normalize - its still a log, which aren't H1
	quaternion2double(xyzw,&qadded);
	vecscaled(xyzw,xyzw,-.25); // -ve and /4
	xyzw[3] *= -.25;  //I think this will still be 0 after adding 2 logs

	//compute exp
	double2quaternion(&qexp,xyzw);
	quaternion_exp(&qexp,&qexp);

	//quaternion_normalize(&qexp); //can normalize now, back to H1
	quaternion_multiply(si,qi,&qexp);

}

void debug_SquadOrientationInterpolator(struct X3D_SquadOrientationInterpolator *px){
	//just a mess of crap, plus: normalization of axisangle axis
	// qinterp = Squad(qi, qi+1,si,si+1,h) = Slerp( Slerp(qi,qi+1,h), Slerp(si,si+1,h), 2h(1-h))
	//in theory, the vrmlrot_to_quaternion and compute_si can be done in a compile_squadorientationinterpolator step
	//and the si and quats for each keyvalue cached
	Quaternion qi,qip1,qip2,qim1,si,sip1,di,dip1,qs,ss;
	double h, xyzw1[4],xyzw2[4];
	int ip1, ip2, im1, kin, iend;
	struct SFRotation *kVs = px->keyValue.p;
	static int once_debug = 0;
	if(once_debug != 0) return;
	once_debug = 1;
	kin = px->key.n;

	//for wrap-around if closed, adjust the wrap around end value based on 
	//whether the scene author duplicated the first keyvalue in the last.
	iend = kin;
	if(vecsame4f(px->keyValue.p[0].c,px->keyValue.p[kin-1].c)) iend = kin -1;

	//there are 1 fewer spans than key/values
	//we'll iterate over key/values here
	printf("Si:\n");
	for(int i=0;i<kin;i++){
		float axislength;
		if(1) printf("%d ri   [%f %f %f, %f]\n",i,kVs[i].c[0],  kVs[i].c[1],   kVs[i].c[2],   kVs[i].c[3]);
		axislength = veclength3f(kVs[i].c);
		if(axislength != 0.0f)
			vecscale3f(kVs[i].c,kVs[i].c,1.0f/axislength);
		if(1) printf("%d ri   [%f %f %f, %f]\n",i,kVs[i].c[0],  kVs[i].c[1],   kVs[i].c[2],   kVs[i].c[3]);
			
		vrmlrot_to_quaternion (&qi,   kVs[i].c[0],  kVs[i].c[1],   kVs[i].c[2],   kVs[i].c[3]);
		ip1 = i+1;
		ip1 = px->closed ? iwrap(ip1,0,iend) : min(max(ip1,0),kin-2);
		vrmlrot_to_quaternion (&qip1, kVs[ip1].c[0],kVs[ip1].c[1], kVs[ip1].c[2], kVs[ip1].c[3]);
		if(0){
			ip2 =i+2;
			ip2 = px->closed ? iwrap(ip2,0,iend) : min(max(ip2,0),kin-1);
			vrmlrot_to_quaternion (&qip2, kVs[ip2].c[0],kVs[ip2].c[1], kVs[ip2].c[2], kVs[ip2].c[3]);
		}
		im1 = i-1;
		im1 = px->closed ? iwrap(im1,0,iend) : min(max(im1,0),kin-3);
		vrmlrot_to_quaternion (&qim1, kVs[im1].c[0],kVs[im1].c[1], kVs[im1].c[2], kVs[im1].c[3]);
		//si
		compute_si(&si,&qi, &qip1, &qim1);
		if(0){
			compute_si(&sip1,&qip1,&qip2,&qi);
			printf("%d qi   [%lf, %lf %lf %lf]\n",i,qi.w,qi.x,qi.y,qi.z);
			printf("%d qi+1 [%lf, %lf %lf %lf]\n",i,qip1.w,qip1.x,qip1.y,qip1.z);
			printf("%d si   [%lf, %lf %lf %lf]\n",i,si.w,si.x,si.y,si.z);
			printf("%d si+1 [%lf, %lf %lf %lf]\n",i,sip1.w,sip1.x,sip1.y,sip1.z);
		}else{
			double xyza[4];
			quaternion_to_vrmlrot(&si,&xyza[0],&xyza[1],&xyza[2],&xyza[3] );
			//printf("%lf %lf %lf %lf, \n",xyza[0],xyza[1],xyza[2],xyza[3]);
		}


	}
	printf("\n");
}

void quaternion_lerp(Quaternion *ret, const Quaternion *q1, const Quaternion *q2, const double t){
	Quaternion t1, t2;
	t1 = *q1;
	t2 = *q2;
	quaternion_scalar_multiply(&t2,t);
	quaternion_scalar_multiply(&t1,1.0 -t);
	quaternion_addition(ret,&t1,&t2);
}
void quaternion_slerp2(Quaternion *ret, const Quaternion *q1, const Quaternion *q2, const double t){
	double dot, dn1,dn2;
	Quaternion r;

	dn1 = quaternion_norm(q1);
	dn2 = quaternion_norm(q2);
	dot = quaternion_dot(q1,q2);
	if(dn1 != 0.0 && dn2 != 0.0) dot = dot /(dn1*dn2); //no improvement to round-the-world or kinks
	r = *q2;
	if(dot < 0.0){
		dot = -dot;
		quaternion_negate(&r);
	}
	if(1.0 - dot < .001){
		quaternion_lerp(ret,q1,&r,t);
	}else{
		Quaternion t1, t2;
		double angle = acos(dot);
		t1 = *q1;
		t2 = r;
		quaternion_scalar_multiply(&t1,sin((1.0-t)*angle));
		quaternion_scalar_multiply(&t2,sin(t*angle));
		quaternion_addition(ret,&t1,&t2);
		// if(ret->w < 0.0) quaternion_negate(ret); //CQRlib Hlerp quirk - no improvement to round-the-world or kinks
		quaternion_scalar_multiply(ret,1.0/sin(angle));
	}

}

void quaternion_squad_prepare(Quaternion *qim1,Quaternion *qi,Quaternion *qip1,Quaternion *qip2,
	Quaternion *s1,Quaternion *s2,Quaternion *qc){
	Quaternion qp,qm, q0,q1,q2,q3;
	q0 = *qim1;
	q1 = *qi;
	q2 = *qip1;
	q3 = *qip2;

	//microsoft directx squad does something like this before computing si, 
	//to avoid round-the-world problems
	//q0 = |q0 + q1| < |q0 - q1| ? -q0 : q0
	//q2 = |q1 + q2| < |q1 - q2| ? -q2 : q2
	//q3 = |q2 + q3| < |q2 - q3| ? -q3 : q3
	quaternion_addition(&qp,&q0,&q1);
	quaternion_subtraction(&qm,&q0,&q1);
	if( quaternion_norm(&qp) < quaternion_norm(&qm) ) quaternion_negate(&q0);
	quaternion_addition(&qp,&q1,&q2);
	quaternion_subtraction(&qm,&q1,&q2);
	if( quaternion_norm(&qp) < quaternion_norm(&qm) ) quaternion_negate(&q2);
	quaternion_addition(&qp,&q2,&q3);
	quaternion_subtraction(&qm,&q2,&q3);
	if( quaternion_norm(&qp) < quaternion_norm(&qm) ) quaternion_negate(&q3);

	compute_si(s1,&q1,&q2,&q0);
	compute_si(s2,&q2,&q3,&q1);
	*qc = q2; //qip1 could have changed sign, use the sign-changed version in squad slerping

}
void quaternion_squad(Quaternion *final,Quaternion *q1,Quaternion *q2, Quaternion *s1,Quaternion *s2,double t){
	Quaternion qs, ss;
	quaternion_slerp2(&qs,q1,q2,t); //qip1 replaceed with possibly negated qc, helps test D
	quaternion_slerp2(&ss,s1,s2,t);
	quaternion_slerp2(final, &qs, &ss, 2.0*t*(1.0 -t));
	quaternion_normalize(final);
}

void do_SquadOrientationInterpolator(void *node){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/interp.html#SquadOrientationInterpolator
	// EXCEPT: specs seem to have a few things wrong
	//   1. there's no 'velocity' stuff needed
	//   2. no relation to the spine interpolators
	// Squad research:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb281656(v=vs.85).aspx
	// - Squad interp using slerps
	//   Slerp(Slerp(q1, c, t), Slerp(a, b, t), 2t(1 - t))
	// https://theory.org/software/qfa/writeup/node12.html
	// - Also squad via slerp
	//   squad(b0, S1, S2, b3, u) = slerp( slerp(b0,b3,u), slerp(S1, S2, u), 2u(1-u))
	// http://run.usc.edu/cs520-s13/assign2/p245-shoemake.pdf
	// - SHOE paper refered to by web3d specs, no mention of squad, uses bezier slerps.
	// http://web.mit.edu/2.998/www/QuaternionReport1.pdf  
	// - page 51 explains SHOE, 
	// qinterp = Squad(qi, qi+1,si,si+1,h) = Slerp( Slerp(qi,qi+1,h), Slerp(si,si+1,h), 2h(1-h))
	//		where si = qi * exp( - (log(inv(qi)*qi+1) + log(inv(qi)*qi-1)) / 4 )
	//		and qi+1 means q[i+1] and qi-1 means q[i-1] ie that's an indexer, not a quat + scalar
	//	p. 15 shows log(q) if q=[cost,sint*v] then 
	//			log(q) = [0, sint*v] (non-unit)
	//			exp(q) = [cost, sint*v] (puts cos(t) back in scalar part, inverting log)
	//						remember from trig cos**2 + sin**2 = 1, or cos = sqrt(1 - sin**2)
	//						can probably take sine from sint*v ie sint = veclength(sint*v)
	// http://www.cs.ucr.edu/~vbz/resources/quatut.pdf
	// p.10 "the logarithm of a unit quaternion q = [vˆ sin O, cos O] is just vˆO, a pure vector of length O."
	//	July 19, 2016 - code below copied from orientationInterpolator and node caste changed, but no other changes
	//  Dec 25, 2016 - squad coded


	struct X3D_SquadOrientationInterpolator *px;
	int kin, kvin;
	struct SFRotation *kVs;
	int counter, iend;
	float interval;		/* where we are between 2 values */
	// UNUSED?? int stzero;
	// UNUSED?? int endzero;	/* starting and/or ending angles zero? */

	Quaternion st, fin, final;
	double x,y,z,a;

	if (!node) return;
	px = (struct X3D_SquadOrientationInterpolator *) node;
	kin = ((px->key).n);
	kvin = ((px->keyValue).n);
	kVs = ((px->keyValue).p);


	#ifdef SEVERBOSE
	printf ("starting do_Oint4; keyValue count %d and key count %d\n",
				kvin, kin);
	#endif
	if(0) debug_SquadOrientationInterpolator(px);

	MARK_EVENT (node, offsetof (struct X3D_SquadOrientationInterpolator, value_changed));

	/* make sure we have the keys and keyValues */
	if ((kvin == 0) || (kin == 0)) {
		px->value_changed.c[0] = (float) 0.0;
		px->value_changed.c[1] = (float) 0.0;
		px->value_changed.c[2] = (float) 0.0;
		px->value_changed.c[3] = (float) 0.0;
		return;
	}
	if (kin>kvin) kin=kvin; /* means we don't use whole of keyValue, but... */
	//for wrap-around if closed, adjust the wrap around end value based on 
	//whether the scene author duplicated the first keyvalue in the last.
	iend = kin;
	if(vecsame4f(px->keyValue.p[0].c,px->keyValue.p[kin-1].c)) iend = kin -1;


	/* set_fraction less than or greater than keys */
	if (px->set_fraction <= ((px->key).p[0])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[0], sizeof (struct SFRotation));
	} else if (px->set_fraction >= ((px->key).p[kin-1])) {
		memcpy ((void *)&px->value_changed,
				(void *)&kVs[kvin-1], sizeof (struct SFRotation));
	} else {
		counter = find_key(kin,(float)(px->set_fraction),px->key.p);
		interval = (px->set_fraction - px->key.p[counter-1]) /
				(px->key.p[counter] - px->key.p[counter-1]);

		
		/* are either the starting or ending angles zero? */
		// unused? stzero = APPROX(kVs[counter-1].c[3],0.0);
		// unused? endzero = APPROX(kVs[counter].c[3],0.0);
		#ifdef SEVERBOSE
			printf ("counter %d interval %f\n",counter,interval);
			printf ("angles %f %f %f %f, %f %f %f %f\n",
				kVs[counter-1].c[0],
				kVs[counter-1].c[1],
				kVs[counter-1].c[2],
				kVs[counter-1].c[3],
				kVs[counter].c[0],
				kVs[counter].c[1],
				kVs[counter].c[2],
				kVs[counter].c[3]);
		#endif
		//squad
		// qinterp = Squad(qi, qi+1,si,si+1,h) = Slerp( Slerp(qi,qi+1,h), Slerp(si,si+1,h), 2h(1-h))
		//in theory, the vrmlrot_to_quaternion and compute_si can be done in a compile_squadorientationinterpolator step
		//then just quaternion_slerps here
		Quaternion qi,qip1,qip2,qim1,si,sip1,qs,ss;
		double h;
		int ip1, ip2, im1, i;
		i = counter -1;
		vrmlrot_to_quaternion (&qi,   kVs[i].c[0],  kVs[i].c[1],   kVs[i].c[2],   kVs[i].c[3]);
		quaternion_normalize(&qi);
		ip1 = i+1;
		ip1 = px->closed ? iwrap(ip1,0,iend) : min(max(ip1,0),kin-2);
		vrmlrot_to_quaternion (&qip1, kVs[ip1].c[0],kVs[ip1].c[1], kVs[ip1].c[2], kVs[ip1].c[3]);
		quaternion_normalize(&qip1);
		ip2 =i+2;
		ip2 = px->closed ? iwrap(ip2,0,iend) : min(max(ip2,0),kin-1);
		vrmlrot_to_quaternion (&qip2, kVs[ip2].c[0],kVs[ip2].c[1], kVs[ip2].c[2], kVs[ip2].c[3]);
		quaternion_normalize(&qip2);
		im1 = i-1;
		im1 = px->closed ? iwrap(im1,0,iend) : min(max(im1,0),kin-3);
		vrmlrot_to_quaternion (&qim1, kVs[im1].c[0],kVs[im1].c[1], kVs[im1].c[2], kVs[im1].c[3]);
		quaternion_normalize(&qim1);

		//quaternion_squad_prepare(qim1,qi,qip1,qip2,s1,s2,q2);
		//si
		compute_si(&si,&qi, &qip1, &qim1);
		compute_si(&sip1,&qip1,&qip2,&qi);
		h = (double) interval;

		Quaternion qc;
		quaternion_squad_prepare(&qim1,&qi,&qip1,&qip2,&si,&sip1,&qc);
		quaternion_squad(&final,&qi,&qc,&si,&sip1,h);

		quaternion_to_vrmlrot(&final,&x, &y, &z, &a);
		px->value_changed.c[0] = (float) x;
		px->value_changed.c[1] = (float) y;
		px->value_changed.c[2] = (float) z;
		px->value_changed.c[3] = (float) a;

		#ifdef SEVERBOSE
		printf ("Oint, new angle %f %f %f %f\n",px->value_changed.c[0],
			px->value_changed.c[1],px->value_changed.c[2], px->value_changed.c[3]);
		#endif
	}

}

//END MIT LIC <<<<<<<