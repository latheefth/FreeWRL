
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
		vecset2f(px->value_changed.c,0.0f,0.0f,0.0f);
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

void compute_si(Quaternion *si,Quaternion *qi, Quaternion *qip1, Quaternion *qim1){
	//		si = qi * exp( - (log(inv(qi)*qi+1) + log(inv(qi)*qi-1)) / 4 )
	//		and qi+1 means q[i+1] and qi-1 means q[i-1] ie that's an indexer, not a quat + scalar
	//	p. 15 shows log(q) if q=[cost,sint*v] then 
	//			log(q) = [0, sint*v] (non-unit)
	//			exp(q) = [cost, sint*v] (puts cos(t) back in scalar part, inverting log)

	Quaternion qiinv, qiinv_qip1, qiinv_qim1,qadded,qexp;
	double xyz[3],sine,cosine;

	quaternion_inverse(&qiinv,qi);
	quaternion_multiply(&qiinv_qip1,&qiinv,qip1);
	qiinv_qip1.w = 0.0; //log
	quaternion_multiply(&qiinv_qim1,&qiinv,qim1);
	qiinv_qim1.w = 0.0; //log
	quaternion_add(&qadded,&qiinv_qip1,&qiinv_qim1);
	qadded.x /= 4.0;
	qadded.y /= 4.0;
	qadded.z /= 4.0;
	// remember from trig cos**2 + sin**2 = 1, or cos = sqrt(1 - sin**2)
	// can probably take sine from sint*v ie sint = veclength(sint*v)
	xyz[0] = qadded.x;
	xyz[1] = qadded.y;
	xyz[2] = qadded.z;
	sine = veclengthd(xyz);
	cosine = sqrt(1.0 - sine*sine);
	qexp = qadded;
	qexp.w = cosine;
	quaternion_multiply(si,qi,&qexp);
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
	// - SHOE paper refered to by web3d specs, 'un-readable jibberish' (dug9)
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
	//	July 19, 2016 - code below copied from orientationInterpolator and node caste changed, but no other changes
	//  Dec 25, 2016 - squad coded


	struct X3D_SquadOrientationInterpolator *px;
	int kin, kvin;
	struct SFRotation *kVs;
	int counter;
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
		if(0){
			//regular slerp code from orientaiton interpolator
			vrmlrot_to_quaternion (&st, kVs[counter-1].c[0],
									kVs[counter-1].c[1], kVs[counter-1].c[2], kVs[counter-1].c[3]);
			vrmlrot_to_quaternion (&fin,kVs[counter].c[0],
									kVs[counter].c[1], kVs[counter].c[2], kVs[counter].c[3]);

			quaternion_slerp(&final, &st, &fin, (double)interval);
		}else{
			//squad
			// qinterp = Squad(qi, qi+1,si,si+1,h) = Slerp( Slerp(qi,qi+1,h), Slerp(si,si+1,h), 2h(1-h))

			Quaternion qi,qip1,qip2,qim1,si,sip1,qs,ss;
			double h;
			int ip1, ip2, im1, i;
			i = counter -1;
			vrmlrot_to_quaternion (&qi,   kVs[i].c[0],  kVs[i].c[1],   kVs[i].c[2],   kVs[i].c[3]);
			ip1 = i+1;
			vrmlrot_to_quaternion (&qip1, kVs[ip1].c[0],kVs[ip1].c[1], kVs[ip1].c[2], kVs[ip1].c[3]);
			ip2 = i+2;
			vrmlrot_to_quaternion (&qip1, kVs[ip2].c[0],kVs[ip2].c[1], kVs[ip2].c[2], kVs[ip2].c[3]);
			im1 = i-1;
			vrmlrot_to_quaternion (&qim1, kVs[im1].c[0],kVs[im1].c[1], kVs[im1].c[2], kVs[im1].c[3]);
			//si
			compute_si(&si,&qi, &qip1, &qim1);
			compute_si(&sip1,&qip1,&qip2,&qi);
			h = (double) interval;
			quaternion_slerp(&qs,&qi,&qip1,h);
			quaternion_slerp(&ss,&si,&sip1,h);
			quaternion_slerp(&final, &qs, &ss, 2*h*(1.0 -h));

		}
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