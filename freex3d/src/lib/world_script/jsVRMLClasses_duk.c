/*

  A substantial amount of code has been adapted from js/src/js.c,
  which is the sample application included with the javascript engine.

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
#include <config.h>
#if defined(JAVASCRIPT_DUK)
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>

#include <io_files.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../main/Snapshot.h"
#include "../scenegraph/LinearAlgebra.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"
#include "../input/InputFunctions.h"

#include "FWTYPE.h"
#include "JScript.h"
#include "CScripts.h"

#define LARGESTRING 2048
#define STRING 512
#define SMALLSTRING 128


/********************************************************/
/*							*/
/* Second part - SF classes				*/
/*							*/
/********************************************************/

/*
Notes for the virtual proxy approach used here:
The field types represented by primitives in ecmascript have no constructor,
 gc=0 on their pointer, and getter/setter convert to/from ecma primitives:
 SFBool		boolean
 SFInt32	numeric
 SFFloat	numeric
 SFTime		numeric
 SFDouble	numeric
 SFString	string
For Script node fields they show up as all other field types as properties on the js context global object.
But unlike other field types, there's no new SFBool(). Getters and setters convert to/from ecma primitives
and set the valueChanged flag when set. Similarly other fieldtype functions and getter/setters convert to/from
ecma primitive instead of one of the above, and never generate a new one of these.
*/

int type2SF(int itype);

/* from http://www.cs.rit.edu/~ncs/color/t_convert.html */
double MIN(double a, double b, double c) {
	double min;
	if((a<b)&&(a<c))min=a; else if((b<a)&&(b<c))min=b; else min=c; return min;
}

double MAX(double a, double b, double c) {
	double max;
	if((a>b)&&(a>c))max=a; else if((b>a)&&(b>c))max=b; else max=c; return max;
}

void convertRGBtoHSV(double r, double g, double b, double *h, double *s, double *v) {
	double my_min, my_max, delta;

	my_min = MIN( r, g, b );
	my_max = MAX( r, g, b );
	*v = my_max;				/* v */
	delta = my_max - my_min;
	if( my_max != 0 )
		*s = delta / my_max;		/* s */
	else {
		/* r = g = b = 0 */		/* s = 0, v is undefined */
		*s = 0;
		*h = -1;
		return;
	}
	if( r == my_max )
		*h = ( g - b ) / delta;		/* between yellow & magenta */
	else if( g == my_max )
		*h = 2 + ( b - r ) / delta;	/* between cyan & yellow */
	else
		*h = 4 + ( r - g ) / delta;	/* between magenta & cyan */
	*h *= 60;				/* degrees */
	if( *h < 0 )
		*h += 360;
}
void convertHSVtoRGB( double h, double s, double v ,double *r, double *g, double *b)
{
	int i;
	double f, p, q, t;
	if( s == 0 ) {
		/* achromatic (grey) */
		*r = *g = *b = v;
		return;
	}
	h /= 60;			/* sector 0 to 5 */
	i = (int) floor( h );
	f = h - i;			/* factorial part of h */
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0: *r = v; *g = t; *b = p; break;
		case 1: *r = q; *g = v; *b = p; break;
		case 2: *r = p; *g = v; *b = t; break;
		case 3: *r = p; *g = q; *b = v; break;
		case 4: *r = t; *g = p; *b = v; break;
		default: *r = v; *g = p; *b = q; break;
	}
}


int SFColor_getHSV(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//argc should == 0 for getHSV
	struct SFColor *ptr = (struct SFColor *)fwn;
	double xp[3];
	/* convert rgb to hsv */
	convertRGBtoHSV((double)ptr->c[0], (double)ptr->c[1], (double)ptr->c[2],&xp[0],&xp[1],&xp[2]);
	//supposed to return numeric[3] - don't have that set up so sfvec3d
	struct SFVec3d *sf3d = malloc(sizeof(struct SFVec3d)); //garbage collector please
	memcpy(sf3d->c,xp,sizeof(double)*3);
	fwretval->_web3dval.native = sf3d; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFColor_setHSV(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//argc should == 3 for setHSV
	struct SFColor *ptr = (struct SFColor *)fwn;
	double xp[3];
	/* convert rgb to hsv */
	convertHSVtoRGB((double)fwpars[0]._numeric, (double)fwpars[1]._numeric, (double)fwpars[2]._numeric,&xp[0],&xp[1],&xp[2]);
	ptr->c[0] = (float)xp[0];
	ptr->c[1] = (float)xp[1];
	ptr->c[2] = (float)xp[2];
	return 0;
}


int SFFloat_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	float *ptr = (float *)fwn;
	//fwretval->_numeric =  (double)*(ptr);
	fwretval->_web3dval.native = ptr;
	fwretval->_web3dval.fieldType = FIELDTYPE_SFFloat;
	fwretval->itype = 'W';
	return 1;
}
int SFFloat_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	float *ptr = (float *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	//(*ptr) = (float)fwval->_numeric;
	//if(fwval->itype == 'F')
		(*ptr) = (float)fwval->_numeric;
	//else if(fwval->itype = 'W')
	//	(*ptr) = fwval->_web3dval.anyvrml->sffloat;

	return TRUE;
}


//#define FIELDTYPE_SFFloat	0
FWTYPE SFFloatType = {
	FIELDTYPE_SFFloat,
	"SFFloat",
	sizeof(float), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	SFFloat_Getter, //Getter,
	SFFloat_Setter, //Setter,
	0,0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFFloat	1
//FWTYPE MFFloatType = {
//	FIELDTYPE_MFFloat,
//	"MFFloat",
//	sizeof(struct Multi_Any), //sizeof(struct ), 
//	NULL, //constructor
//	NULL, //constructor args
//	NULL, //Properties,
//	NULL, //special iterator
//	NULL, //Getter,
//	NULL, //Setter,
//	'N',0, //index prop type,readonly
//	NULL, //functions
//};

//MFW for MF types that take web3d (non-ecma-primitive) types ie new MFColor( new SFColor(0,0,0), new SFColor(.1,.2,.3), ...) 
ArgListType (MFW_ConstructorArgs)[] = {
		{0,0,0,"W"},
		{-1,0,0,NULL},
};
int sizeofSF(int itype); //thunks MF to SF (usually itype-1) and gets sizeof SF
void * MFW_Constructor(FWType fwtype, int argc, FWval fwpars){
	int lenSF;
	struct Multi_Any *ptr = malloc(sizeof(struct Multi_Any));  ///malloc in 2 parts for MF
	lenSF = sizeofSF(fwtype->itype); 
	ptr->n = argc;
	ptr->p = NULL;
	if(ptr->n)
		ptr->p = malloc(ptr->n * lenSF); // This second part is resizable ie MF[i] = new SF() if i >= (.length), .length is expanded to accomodate
	char *p = ptr->p;
	for(int i=0;i<ptr->n;i++){
		memcpy(p,fwpars[i]._web3dval.native,lenSF);
		p += lenSF;
	}
	return (void *)ptr;
}
FWPropertySpec (MFW_Properties)[] = {
	{"length", -1, 'I', 0},
	{NULL,0,0,0},
};
int MFW_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		//length
		fwretval->_integer = ptr->n;
		fwretval->itype = 'I';
		//fwretval->_web3dval.native = &ptr->n;
		//fwretval->_web3dval.fieldType = FIELDTYPE_SFInt32;
		//fwretval->itype = 'W';
		nr = 1;
	}else if(index > -1 && index < ptr->n){
		int elen = sizeofSF(fwt->itype);
		fwretval->_web3dval.native = (void *)(ptr->p + index*elen);
		fwretval->_web3dval.fieldType = type2SF(fwt->itype);
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}
int MFW_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
	int nelen, nold, nr = FALSE;
	int elen = sizeofSF(fwt->itype);
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		//length
		nold = ptr->n;
		//ptr->n = fwval->_integer;
		//if(fwval->itype == 'I')
			ptr->n = fwval->_integer;
		//else if(fwval->itype == 'W')
		//	ptr->n = fwval->_web3dval.anyvrml->sfint32;
		if(ptr->n > nold){
			//nelen = (int) upper_power_of_two(fwval->_integer);
			ptr->p = realloc(ptr->p,ptr->n * elen);
		}
		nr = TRUE;
	}else if(index > -1){
		if(index >= ptr->n){
			//nold = ptr->n;
			ptr->n = index+1;
			//nelen = (int) upper_power_of_two(ptr->n);
			ptr->p = realloc(ptr->p,ptr->n *elen); //need power of 2 if SFNode children
		}
		memcpy(ptr->p + index*elen,fwval->_web3dval.native, elen);
		nr = TRUE;
	}
	return nr;
}

void * MFFloat_Constructor(FWType fwtype, int argc, FWval fwpars){
	int lenSF;
	struct Multi_Any *ptr = malloc(sizeof(struct Multi_Any));  ///malloc in 2 parts for MF
	lenSF = sizeofSF(fwtype->itype); 
	ptr->n = argc;
	ptr->p = NULL;
	if(ptr->n)
		ptr->p = malloc(ptr->n * lenSF); // This second part is resizable ie MF[i] = new SF() if i >= (.length), .length is expanded to accomodate
	char *p = ptr->p;
	for(int i=0;i<ptr->n;i++){
		//float ff = (float)fwpars[i]._numeric; //fwpars[i]._web3dval.native;
		memcpy(p,&fwpars[i]._web3dval.native,lenSF);
		p += lenSF;
	}
	return (void *)ptr;
}
ArgListType (MFFloat_ConstructorArgs)[] = {
		{0,0,0,"F"},
		{-1,0,0,NULL},
};

FWTYPE MFFloatType = {
	FIELDTYPE_MFFloat,
	"MFFloat",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFFloat_Constructor, //constructor
	MFFloat_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};


// http://www.web3d.org/files/specifications/19777-1/V3.0/Part1/functions.html#SFRotation
/* SFRotation
constructors
SFRotation (numeric x,  numeric y,  numeric z,  numeric angle) x, y, and z are the axis of the rotation. angle is the angle of the rotation (in radians). Missing values default to 0.0, except y, which defaults to 1.0. 
SFRotation (SFVec3f axis,  numeric  angle) axis is the axis of rotation. angle is the angle of the rotation (in radians) 
SFRotation (SFVec3f fromVector,  SFVec3f toVector) fromVector and toVector are normalized and the rotation value that would rotate from the fromVector to the toVector is stored in the object. 
props
numeric x No first value of the axis vector 
numeric y No second value of the axis vector 
numeric z No third value of the axis vector 
numeric angle No the angle of the rotation (in radians) 
funcs
SFVec3f getAxis() Returns the axis of rotation. 
SFRotation inverse() Returns the inverse of this object's rotation.  
SFRotation multiply(SFRotation rot) Returns the object multiplied by the passed value.  
SFVec3f multiVec(SFVec3f vec) Returns the value of vec multiplied by the matrix corresponding to this object's rotation.  
void setAxis(SFVec3f vec) Sets the axis of rotation to the value passed in vec.  
SFRotation slerp(SFRotation dest, numeric t) Returns the value of the spherical linear interpolation between this object's rotation and dest at value 0 = t = 1. For t = 0, the value is this object`s rotation. For t = 1, the value is dest.  
String toString() Returns a String containing the value of x, y, z, and angle encoding using the X3D Classic VRML encoding (see part 2 of ISO/IEC 19776). 
*/
int SFRotation_getAxis(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFRotation *ptr = (struct SFRotation *)fwn;
	struct SFVec3f *res = malloc(sizeof(struct SFVec3f));
	veccopy3f(res->c,ptr->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFRotation_inverse(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFRotation *ptr = (struct SFRotation *)fwn;
	struct SFRotation *res = malloc(sizeof(struct SFRotation));
	Quaternion q1,qret;
	double a,b,c,d;

	/* convert both rotation to quaternion */
	vrmlrot_to_quaternion(&q1, (double) ptr->c[0], 
		(double) ptr->c[1], (double) ptr->c[2], (double) ptr->c[3]);

	/* invert it */
	quaternion_inverse(&qret,&q1);

	/* and return the resultant, as a vrml rotation */
	quaternion_to_vrmlrot(&qret, &a, &b, &c, &d);
	/* double to floats, can not use pointers... */
	res->c[0] = (float) a;
	res->c[1] = (float) b;
	res->c[2] = (float) c;
	res->c[3] = (float) d;

	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFRotation;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFRotation_multiply(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFRotation *ptr = (struct SFRotation *)fwn;
	struct SFRotation *rhs = (struct SFRotation *)fwpars[0]._web3dval.native;
	struct SFRotation *res = malloc(sizeof(struct SFRotation));
	Quaternion q1,q2,qret;
	double a,b,c,d;

	/* convert both rotation to quaternion */
	vrmlrot_to_quaternion(&q1, (double) ptr->c[0], 
		(double) ptr->c[1], (double) ptr->c[2], (double) ptr->c[3]);

	vrmlrot_to_quaternion(&q2, (double) rhs->c[0], 
		(double) rhs->c[1], (double) rhs->c[2], (double) rhs->c[3]);

	/* multiply them */
	quaternion_multiply(&qret,&q1,&q2);

	/* and return the resultant, as a vrml rotation */
	quaternion_to_vrmlrot(&qret, &a, &b, &c, &d);
	/* double to floats, can not use pointers... */
	res->c[0] = (float) a;
	res->c[1] = (float) b;
	res->c[2] = (float) c;
	res->c[3] = (float) d;

	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFRotation;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFRotation_multiVec(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFRotation *ptr = (struct SFRotation *)fwn;
	struct SFVec3f *v = (struct SFVec3f *)fwpars[0]._web3dval.native;
	struct SFVec3f *res = malloc(sizeof(struct SFVec3f));
	struct SFVec3f c1, c2, r;
	double rl,angle,s,c;

	veccopy3f(r.c,ptr->c);
	rl = veclength3f(r.c);
	angle = ptr->c[3];
	s = (float) sin(angle);
	c = (float) cos(angle);
	veccross3f(c1.c,r.c,v->c);
	vecscale3f(c1.c,c1.c,1.0/rl);
	veccross3f(c2.c,r.c,c1.c);
	vecscale3f(c2.c,c2.c,1.0/rl);
	for(int i=0;i<3;i++)
		res->c[i] = (float) (v->c[i] + s * c1.c[i] + (1.0-c) * c2.c[i]);

	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFRotation_setAxis(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFRotation *ptr = (struct SFRotation *)fwn;
	struct SFVec3f *v = (struct SFVec3f *)fwpars[0]._web3dval.native;
	veccopy3f(ptr->c,v->c);
	return 0;
}

int SFRotation_slerp(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFRotation *rot = (struct SFRotation *)fwn;
	struct SFRotation *dest = (struct SFRotation *)fwpars[0]._web3dval.native;
	double t = fwpars[1]._numeric;
	struct SFRotation *res = malloc(sizeof(struct SFRotation));
	Quaternion quat,quat_dest,quat_ret;
	double a,b,c,d;

	/*
	 * From Annex C, C.6.7.4:
	 *
	 * For t = 0, return object's rotation.
	 * For t = 1, return 1st argument.
	 * For 0 < t < 1, compute slerp.
	 */
	if (APPROX(t, 0)) {
		memcpy(res->c,rot->c,4*sizeof(float));
	} else if (APPROX(t, 1)) {
		memcpy(res->c,dest->c,4*sizeof(float));
	} else {

		vrmlrot_to_quaternion(&quat,
							  rot->c[0],
							  rot->c[1],
							  rot->c[2],
							  rot->c[3]);

		vrmlrot_to_quaternion(&quat_dest,
							  dest->c[0],
							  dest->c[1],
							  dest->c[2],
							  dest->c[3]);

		quaternion_slerp(&quat_ret, &quat, &quat_dest, t);
		quaternion_to_vrmlrot(&quat_ret,&a,&b,&c,&d);
		/* double to floats, can not use pointers... */
		res->c[0] = (float) a;
		res->c[1] = (float) b;
		res->c[2] = (float) c;
		res->c[3] = (float) d;
	}
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFRotation;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFRotation_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFRotation *ptr = (struct SFRotation *)fwn;
	char buff[STRING], *str;
	int len;
	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g %.9g %.9g",
			ptr->c[0], ptr->c[1], ptr->c[2], ptr->c[3]);
	len = strlen(buff);
	str = malloc(len+1);  //leak
	strcpy(str,buff);
	fwretval->_string = str;
	fwretval->itype = 'S';
	return 1;
}


FWFunctionSpec (SFRotation_Functions)[] = {
	{"getAxis", SFRotation_getAxis, 'W',{0,-1,0,NULL}},
	{"inverse", SFRotation_inverse, 'W',{1,-1,0,"W"}},
	{"multiply", SFRotation_multiply, 'W',{1,-1,0,"W"}},
	{"multiVec", SFRotation_multiVec, 'W',{1,-1,0,"W"}},
	{"setAxis", SFRotation_setAxis, '0',{1,-1,0,"W"}},
	{"slerp", SFRotation_slerp, 'W',{2,-1,0,"WF"}},
	{"toString", SFRotation_toString, 'S',{0,-1,0,NULL}},
	{0}
};
int SFRotation_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		nr = 1;
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
		case 3: //angle
			fwretval->_numeric =  ptr->c[index];
			//fwretval->_web3dval.anyvrml = (union anyVrml*)&ptr->c[index];
			//fwretval->_web3dval.fieldType = FIELDTYPE_SFFloat;
			break;
		default:
			nr = 0;
		}
	}
	fwretval->itype = 'F';
	return nr;
}
int SFRotation_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
		case 3: //angle
			ptr->c[index] = (float)fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFRotation_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFRotation *ptr = malloc(fwtype->size_of); //garbage collector please
	if(ic == 4){
		for(int i=0;i<4;i++)
			ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	}else if(ic == 2 && fwpars[1].itype == 'F'){
		//SFVec3f axis, float angle
		veccopy3f(ptr->c,fwpars[0]._web3dval.native);
		ptr->c[3] = (float)fwpars[1]._numeric;

	}else if(ic == 2 && fwpars[1].itype == 'W'){
		//SFVec3f from SFVec3f to
		struct SFVec3f *v1 = fwpars[0]._web3dval.native;
		struct SFVec3f *v2 = fwpars[1]._web3dval.native;
		float v1len = veclength3f(v1->c);
		float v2len = veclength3f(v2->c);
		float v12dp = vecdot3f(v1->c, v2->c);
		veccross3f(ptr->c,v1->c,v2->c);
		v12dp /= v1len * v2len;
		ptr->c[3] = (float) atan2(sqrt(1 - v12dp * v12dp), v12dp);
	}
	return (void *)ptr;
}
FWPropertySpec (SFRotation_Properties)[] = {
	{"x", 0, 'F', 0},
	{"y", 1, 'F', 0},
	{"z", 2, 'F', 0},
	{"angle", 3, 'F', 0},
	{NULL,0,0,0},
};
ArgListType (SFRotation_ConstructorArgs)[] = {
	{4,0,'T',"FFFF"},
	{2,0,'T',"WF"},
	{2,-1,'F',"WW"},
	{-1,0,0,NULL},
};
//#define FIELDTYPE_SFRotation	2
FWTYPE SFRotationType = {
	FIELDTYPE_SFRotation,
	"SFRotation",
	sizeof(struct SFRotation), //sizeof(struct ), 
	SFRotation_Constructor, //constructor
	SFRotation_ConstructorArgs, //constructor args
	SFRotation_Properties, //Properties,
	NULL, //special iterator
	SFRotation_Getter, //Getter,
	SFRotation_Setter, //Setter,
	'F',0, //index prop type,readonly
	SFRotation_Functions, //functions
};



//#define FIELDTYPE_MFRotation	3
FWTYPE MFRotationType = {
	FIELDTYPE_MFRotation,
	"MFRotation",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};



/*
SFVec3f add(SFVec3f vec) Returns the value of the passed value added, component-wise, to the object. 
SFVec3f cross(SFVec3f vec) Returns the cross product of the object and the passed value. 
SFVec3f divide(numeric n) Returns the value of the object divided by the passed value. 
numeric dot(SFVec3f vec) Returns the dot product of this vector and the passed value as a double precision value. 
numeric length() Returns the geometric length of this vector as a double precision value. 
SFVec3f multiple(numeric n) Returns the value of the object multiplied by the passed value. 
SFVec3f negate() Returns the value of the component-wise negation of the object. 
SFVec3f normalize() Returns the object converted to unit length . 
SFVec3f subtract(SFVec3f vec) Returns the value of the passed value subtracted, component-wise, from the object. 
String toString() Returns a String containing the value of x, y, and z encoded using the X3D Classic VRML encoding (see part 2 of ISO/IEC 19776). 
*/
#include "../scenegraph/LinearAlgebra.h"
//SFVec3f add(SFVec3f vec) Returns the value of the passed value added, component-wise, to the object. 
int SFVec3f_add(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	struct SFVec3f *rhs = fwpars[0]._web3dval.native;
	struct SFVec3f *res = malloc(fwtype->size_of);
	vecadd3f(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3f_cross(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	struct SFVec3f *rhs = fwpars[0]._web3dval.native;
	struct SFVec3f *res = malloc(fwtype->size_of);
	veccross3f(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3f_subtract(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	struct SFVec3f *rhs = fwpars[0]._web3dval.native;
	struct SFVec3f *res = malloc(fwtype->size_of);
	vecdif3f(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3f_divide(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	float rhs = fwpars[0]._numeric;
	if(rhs == 0.0f){
		return 0;
	}
	rhs = 1.0/rhs;
	struct SFVec3f *res = malloc(fwtype->size_of);
	vecscale3f(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3f_multiply(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	float rhs = fwpars[0]._numeric;
	struct SFVec3f *res = malloc(fwtype->size_of);
	vecscale3f(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3f_normalize(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	struct SFVec3f *res = malloc(fwtype->size_of);
	vecnormalize3f(res->c,ptr->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFVec3f_negate(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	struct SFVec3f *res = malloc(fwtype->size_of);
	vecscale3f(res->c,ptr->c,-1.0f);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3f_length(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	float res;
	res = veclength3f(ptr->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'F';
	return 1;
}
int SFVec3f_dot(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	struct SFVec3f *rhs = fwpars[0]._web3dval.native;
	double res;
	res = vecdot3f(ptr->c,rhs->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'F';
	return 1;
}

FWFunctionSpec (SFVec3f_Functions)[] = {
	{"add", SFVec3f_add, 'W',{1,-1,0,"W"}},
	{"cross", SFVec3f_cross, 'W',{1,-1,0,"W"}},
	{"divide", SFVec3f_divide, 'W',{1,-1,0,"F"}},
	{"dot", SFVec3f_dot, 'F',{1,-1,0,"W"}},
	{"length", SFVec3f_length, 'F',{0,-1,0,NULL}},
	{"multiply", SFVec3f_multiply, 'W',{1,-1,0,"F"}},
	{"negate", SFVec3f_negate, 'W',{0,-1,0,NULL}},
	{"normalize", SFVec3f_normalize, 'W',{0,-1,0,NULL}},
	{"subtract", SFVec3f_subtract, 'W',{1,-1,0,"W"}},
	//{"toString", SFVec3f_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int SFVec3f_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		nr = 1;
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
			fwretval->_numeric =  ptr->c[index];
			//fwretval->_web3dval.anyvrml = (union anyVrml*)&ptr->c[index];
			//fwretval->_web3dval.fieldType = FIELDTYPE_SFFloat;
			break;
		default:
			nr = 0;
		}
	}
	fwretval->itype = 'F';
	return nr;
}
int SFVec3f_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
			ptr->c[index] = (float)fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFVec3f_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec3f *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<3;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFVec3f_Properties)[] = {
	{"x", 0, 'F', 0},
	{"y", 1, 'F', 0},
	{"z", 2, 'F', 0},
	{NULL,0,0,0},
};
ArgListType (SFVec3f_ConstructorArgs)[] = {
		{3,0,'T',"FFF"},
		{-1,0,0,NULL},
};


//#define FIELDTYPE_SFVec3f	4
FWTYPE SFVec3fType = {
	FIELDTYPE_SFVec3f,
	"SFVec3f",
	sizeof(struct SFVec3f), //sizeof(struct ), 
	SFVec3f_Constructor, //constructor
	SFVec3f_ConstructorArgs, //constructor args
	SFVec3f_Properties, //Properties,
	NULL, //special iterator
	SFVec3f_Getter, //Getter,
	SFVec3f_Setter, //Setter,
	'F',0, //index prop type,readonly
	SFVec3f_Functions, //functions
};

//#define FIELDTYPE_MFVec3f	5
FWTYPE MFVec3fType = {
	FIELDTYPE_MFVec3f,
	"MFVec3f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFBool	6
FWTYPE SFBoolType = {
	FIELDTYPE_SFBool,
	"SFBool",
	sizeof(int), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	0,0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFBool	7
FWTYPE MFBoolType = {
	FIELDTYPE_MFBool,
	"MFBool",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'B',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFInt32	8
FWTYPE SFInt32Type = {
	FIELDTYPE_SFInt32,
	"SFInt32",
	sizeof(int), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	0,0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFInt32	9
FWTYPE MFInt32Type = {
	FIELDTYPE_MFInt32,
	"MFInt32",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'I',0, //index prop type,readonly
	NULL, //functions
};

int getFieldFromNodeAndIndex(struct X3D_Node* node, int iifield, const char **fieldname, int *type, int *kind, union anyVrml **value);
int SFNode_Iterator(int index, FWTYPE *fwt, FWPointer *pointer, char **name, int *lastProp, int *jndex, char *type, char *readOnly){
	struct X3D_Node *node = ((union anyVrml*)pointer)->sfnode;
	int ftype, kind, ihave;
	char ctype;
	union anyVrml *value;
	index ++;
	(*jndex) = 0;
	int iifield = index;
	ihave = getFieldFromNodeAndIndex(node, index, name, &ftype, &kind, &value);
	switch(ftype){
		case FIELDTYPE_SFBool: ctype = 'B'; break;
		case FIELDTYPE_SFInt32: ctype = 'I'; break;
		case FIELDTYPE_SFFloat: ctype = 'F'; break;
		case FIELDTYPE_SFDouble: ctype = 'D'; break;
		case FIELDTYPE_SFTime: ctype = 'D'; break;
		case FIELDTYPE_SFString: ctype = 'S'; break;
		default: ctype = 'W'; break;
	}
	if(ihave){
		(*jndex) = index;
		(*lastProp) = index;
		(*type) = ctype;
		//(*readOnly) = kind == PKW_inputOnly ? 'T' : 0;
		(*readOnly) = 0;
		return index;
	}
	return -1;
}
int SFNode_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct X3D_Node *node = ((union anyVrml*)fwn)->sfnode; 
	int ftype, kind, ihave, nr;
	const char *name;
	union anyVrml *value;
	nr = 0;
	ihave = getFieldFromNodeAndIndex(node, index, &name, &ftype, &kind, &value);
	if(ihave){
		fwretval->_web3dval.native = value;
		fwretval->_web3dval.fieldType = ftype;
		fwretval->itype = 'W';
/*
		//copy W type or primative type, depending on ftype
		switch(ftype){
		case FIELDTYPE_SFBool:
			fwretval->itype = 'B';
			fwretval->_boolean = value->sfbool;
			break;
		case FIELDTYPE_SFInt32:
			fwretval->itype = 'I';
			fwretval->_integer = value->sfint32;
			break;

		case FIELDTYPE_SFFloat:
			fwretval->itype = 'F';
			fwretval->_numeric = (double)value->sffloat;
			break;

		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFTime:
			fwretval->itype = 'D';
			fwretval->_numeric = value->sftime;
			break;

		case FIELDTYPE_SFString:
			fwretval->itype = 'S';
			fwretval->_string = value->sfstring->strptr;
			break;

		default:
			fwretval->itype = 'W';
			fwretval->_web3dval.fieldType = ftype;
			fwretval->_web3dval.native = value; //Q. am I supposed to deep copy here? I'm not. So I don't set gc.
		}
*/
		nr = 1;
	}
	return nr;
}
void medium_copy_field0(int itype, void* source, void* dest);
int SFNode_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct X3D_Node *node = ((union anyVrml*)fwn)->sfnode; 
	int ftype, kind, ihave, nr;
	const char *name;
	union anyVrml *value;
	nr = FALSE;
	ihave = getFieldFromNodeAndIndex(node, index, &name, &ftype, &kind, &value);
	if(ihave){
		//value = fwval->_web3dval.anyvrml;  //Q. should be *value = *anyvrml or medium_copy_field?
		//medium_copy_field0(ftype,fwval->_web3dval.native,value);
		//ftype = fwval->_web3dval.fieldType;

		//copy W type or primative type, depending on ftype
		switch(fwval->itype){
		case 'B':
			value->sfbool = fwval->_boolean;
			break;
		case 'I':
			value->sfint32 = fwval->_integer;
			break;

		case 'F':
			value->sffloat = (float)fwval->_numeric;
			break;

		case 'D':
			value->sftime = fwval->_numeric;
			break;

		case 'S':
			value->sfstring = newASCIIString(fwval->_string);
			//value->sfstring->strptr = strdup(fwval->_string);
			//value->sfstring->len = strlen(fwval->_string);
			break;

		default:
			//value = fwval->_web3dval.native; //Q. am I supposed to deep copy here? 
			medium_copy_field0(ftype,fwval->_web3dval.native,value);
		}

		if(node->_nodeType == NODE_Script) {
			//notify for event processing
			struct Shader_Script *script = X3D_SCRIPT(node)->__scriptObj;
			struct ScriptFieldDecl *field;
			field = Shader_Script_getScriptField(script,index);
			if(kind == PKW_inputOutput || kind == PKW_outputOnly)
				field->valueChanged = TRUE;
			if(kind == PKW_inputOnly || kind == PKW_inputOutput) {
				//if using fwsetter0, we could be writing to ourselves, sending ourselves an eventIn when we really just want an eventOut
				//so we should be doing && (gglobal.currentScript != node) and set gglobal.currentScript = thisScriptNode in fwsetter0 (and back to null after call)
				field->eventInSet = TRUE; //see runQueuedDirectOutputs()
			}
		}
		nr = TRUE;
	}
	return nr;

}
void * SFNode_Constructor(FWType fwtype, int nargs, FWval fwpars){
	struct X3D_Node **ptr = NULL; // = malloc(fwtype->size_of); //garbage collector please
	if(nargs == 1){
		if(fwpars[0].itype == 'S'){
			//SFNode.wrl createFromAString = new SFNode('Cylinder {height 1}');
			struct X3D_Group *retGroup;
			char *xstr; 
			char *tmpstr;
			char *separator;
			int ra;
			int count;
			int wantedsize;
			int MallocdSize;
			ttglobal tg = gglobal();
			struct VRMLParser *globalParser = (struct VRMLParser *)tg->CParse.globalParser;
			const char *_c = fwpars[0]._string; // fwpars[0]._web3dval.anyvrml->sfstring->strptr; 

			/* do the call to make the VRML code  - create a new browser just for this string */
			gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
			retGroup = createNewX3DNode(NODE_Group);
			ra = EAI_CreateVrml("String",_c,retGroup);
			globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */
			if(retGroup->children.n < 1) return 0;
			ptr = malloc(sizeof(void *));
			*ptr = retGroup->children.p[0]; 
		}else if(fwpars->itype = 'W'){
			if(fwpars->_web3dval.fieldType == FIELDTYPE_SFNode){
				//SFNode.wrl newedPointer = new SFNode(ptr); 
				ptr = fwpars[0]._web3dval.native; //don't gc
			}
		}
	}
	return ptr;
}
ArgListType (SFNode_ConstructorArgs)[] = {
		{1,-1,0,"S"},
		{1,-1,0,"W"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_SFNode	10
FWTYPE SFNodeType = {
	FIELDTYPE_SFNode,
	"SFNode",
	sizeof(void*), //sizeof(struct ), 
	SFNode_Constructor, //constructor
	SFNode_ConstructorArgs, //constructor args
	NULL, //Properties,
	SFNode_Iterator, //special iterator
	SFNode_Getter, //Getter,
	SFNode_Setter, //Setter,
	0,0, //index prop type,readonly
	NULL, //functions
};



//#define FIELDTYPE_MFNode	11
FWTYPE MFNodeType = {
	FIELDTYPE_MFNode,
	"MFNode",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};


FWFunctionSpec (SFColor_Functions)[] = {
	{"getHSV", SFColor_getHSV, 'W',{0,0,0,NULL}},
	{"setHSV", SFColor_setHSV, 0,{3,-1,'T',"DDD"}},
	{0}
};

int SFColor_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFColor *ptr = (struct SFColor *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		nr = 1;
		switch(index){
		case 0: //r
		case 1: //g
		case 2: //b
			fwretval->_numeric =  ptr->c[index];
			//fwretval->_web3dval.anyvrml = (union anyVrml*)&ptr->c[index];
			//fwretval->_web3dval.fieldType = FIELDTYPE_SFFloat;
			break;
		default:
			nr = 0;
		}
	}
	fwretval->itype = 'F';
	return nr;
}
int SFColor_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFColor *ptr = (struct SFColor *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		switch(index){
		case 0: //r
		case 1: //g
		case 2: //b
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFColor_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFColor *ptr = malloc(fwtype->size_of); //garbage collector please
	if(fwtype->ConstructorArgs[0].nfixedArg == 3)
	for(int i=0;i<3;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFColor_Properties)[] = {
	{"r", 0, 'F', 0},
	{"g", 1, 'F', 0},
	{"b", 2, 'F', 0},
	{NULL,0,0,0},
};

//typedef struct ArgListType {
//	char nfixedArg;
//	char iVarArgStartsAt; //-1 no varargs
//	char fillMissingFixedWithZero; //T/F if F then complain if short
//	char *argtypes; //if varargs, then argtypes[nfixedArg] == type of varArg, and all varargs are assumed the same type
//} ArgListType;
ArgListType (SFColor_ConstructorArgs)[] = {
		{3,0,'T',"FFF"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_SFColor	12
FWTYPE SFColorType = {
	FIELDTYPE_SFColor,
	"SFColor",
	sizeof(struct SFColor), //sizeof(struct ), 
	SFColor_Constructor, //constructor
	SFColor_ConstructorArgs, //constructor args
	SFColor_Properties, //Properties,
	NULL, //special iterator
	SFColor_Getter, //Getter,
	SFColor_Setter, //Setter,
	'F',0, //index prop type,readonly
	SFColor_Functions, //functions
};
//int MFColor_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
//	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
//	int nr = 0;
//	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
//	if(index == -1){
//		//length
//		fwretval->_integer = ptr->n;
//		fwretval->itype = 'I';
//		nr = 1;
//	}else if(index > -1 && index < ptr->n){
//		int elen = sizeofSF(FIELDTYPE_SFColor);
//		fwretval->_web3dval.native = (void *)(ptr->p + index*elen);
//		fwretval->_web3dval.fieldType = FIELDTYPE_SFColor;
//		fwretval->itype = 'W';
//		nr = 1;
//	}
//	return nr;
//}
//int MFColor_Setter(FWType fwt, int index, void * fwn, FWval fwval){
//	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
//	int nr = FALSE;
//	int elen = sizeofSF(FIELDTYPE_SFColor);
//	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
//	if(index == -1){
//		//length
//		ptr->n = fwval->_integer;
//		ptr->p = realloc(ptr->p,ptr->n * elen);
//		nr = TRUE;
//	}else if(index > -1){
//		if(index >= ptr->n){
//			ptr->n = index +1;
//			ptr->p = realloc(ptr->p,ptr->n *elen); //need power of 2 if SFNode
//		}
//		memcpy(ptr->p + index*elen,fwval->_web3dval.native, elen);
//		nr = TRUE;
//	}
//	return nr;
//}
//#define FIELDTYPE_MFColor	13
FWTYPE MFColorType = {
	FIELDTYPE_MFColor,
	"MFColor",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFColorRGBA	14
FWTYPE SFColorRGBAType = {
	FIELDTYPE_SFColorRGBA,
	"SFColorRGBA",
	sizeof(struct SFColorRGBA), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'F',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFColorRGBA	15
FWTYPE MFColorRGBAType = {
	FIELDTYPE_MFColorRGBA,
	"MFColorRGBA",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFTime	16
FWTYPE SFTimeType = {
	FIELDTYPE_SFTime,
	"SFTime",
	sizeof(double), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	0,0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFTime	17
FWTYPE MFTimeType = {
	FIELDTYPE_MFTime,
	"MFTime",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'D',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFString	18
FWTYPE SFStringType = {
	FIELDTYPE_SFString,
	"SFString",
	sizeof(void *), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	0,0, //index prop type,readonly  //Q. should string[i] return 'I' == char, like SFImage indexer?
	NULL, //functions
};

void * MFString_Constructor(FWType fwtype, int argc, FWval fwpars){
	int lenSF;
	struct Multi_Any *ptr = malloc(sizeof(struct Multi_Any));  ///malloc in 2 parts for MF
	lenSF = sizeofSF(fwtype->itype); 
	ptr->n = argc;
	ptr->p = NULL;
	if(ptr->n)
		ptr->p = malloc(ptr->n * lenSF); // This second part is resizable ie MF[i] = new SF() if i >= (.length), .length is expanded to accomodate
	char *p = ptr->p;
	for(int i=0;i<ptr->n;i++){
		//float ff = (float)fwpars[i]._numeric; //fwpars[i]._web3dval.native;
		if(fwpars[i].itype == 'W' && fwpars[i]._web3dval.fieldType == FIELDTYPE_SFString)
			memcpy(p,&fwpars[i]._web3dval.native,lenSF);
		else if(fwpars[i].itype = 'S'){
			void *tmp = newASCIIString(fwpars[i]._string);
			memcpy(p,&tmp,lenSF);
			//(*p) = (char *)
		}
		p += lenSF;
	}
	return (void *)ptr;
}
ArgListType (MFString_ConstructorArgs)[] = {
		{0,0,0,"S"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_MFString	19
FWTYPE MFStringType = {
	FIELDTYPE_MFString,
	"MFString",
	sizeof(struct Multi_Any),
	MFString_Constructor, //constructor
	MFString_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};


//#define FIELDTYPE_SFVec2f	20
FWTYPE SFVec2fType = {
	FIELDTYPE_SFVec2f,
	"SFVec2f",
	sizeof(struct SFVec2f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'F',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec2f	21
FWTYPE MFVec2fType = {
	FIELDTYPE_MFVec2f,
	"MFVec2f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFImage	22
FWTYPE SFImageType = {
	FIELDTYPE_SFImage,
	"SFImage",
	sizeof(void *), //sizeof(struct ),  //----------------------unknown struct but it looks like an SFNode with node-specific fields
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'I',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_FreeWRLPTR	23
//#define FIELDTYPE_FreeWRLThread	24

/*
SFVec3d add(SFVec3d vec) Returns the value of the passed value added, component-wise, to the object. 
SFVec3d cross(SFVec3d vec) Returns the cross product of the object and the passed value. 
SFVec3d divide(numeric n) Returns the value of the object divided by the passed value. 
numeric dot(SFVec3d vec) Returns the dot product of this vector and the passed value as a double precision value. 
numeric length() Returns the geometric length of this vector as a double precision value. 
SFVec3d multiple(numeric n) Returns the value of the object multiplied by the passed value. 
SFVec3d negate() Returns the value of the component-wise negation of the object. 
SFVec3d normalize() Returns the object converted to unit length . 
SFVec3d subtract(SFVec3f vec) Returns the value of the passed value subtracted, component-wise, from the object. 
String toString() Returns a String containing the value of x, y, and z encoded using the X3D Classic VRML encoding (see part 2 of ISO/IEC 19776). 
*/
//SFVec3d add(SFVec3d vec) Returns the value of the passed value added, component-wise, to the object. 
int SFVec3d_add(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	struct SFVec3d *rhs = fwpars[0]._web3dval.native;
	struct SFVec3d *res = malloc(fwtype->size_of);
	vecaddd(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3d_cross(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	struct SFVec3d *rhs = fwpars[0]._web3dval.native;
	struct SFVec3d *res = malloc(fwtype->size_of);
	veccrossd(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3d_subtract(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	struct SFVec3d *rhs = fwpars[0]._web3dval.native;
	struct SFVec3d *res = malloc(fwtype->size_of);
	vecdifd(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3d_divide(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	double rhs = fwpars[0]._numeric;
	if(rhs == 0.0){
		return 0;
	}
	rhs = 1.0/rhs;
	struct SFVec3d *res = malloc(fwtype->size_of);
	vecscaled(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3d_multiply(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	double rhs = fwpars[0]._numeric;
	struct SFVec3d *res = malloc(fwtype->size_of);
	vecscaled(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3d_normalize(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	struct SFVec3d *res = malloc(fwtype->size_of);
	vecnormald(res->c,ptr->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFVec3d_negate(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	struct SFVec3d *res = malloc(fwtype->size_of);
	vecscaled(res->c,ptr->c,-1.0);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec3d_length(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	double res;
	res = veclengthd(ptr->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'D';
	return 1;
}
int SFVec3d_dot(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	struct SFVec3d *rhs = fwpars[0]._web3dval.native;
	double res;
	res = vecdotd(ptr->c,rhs->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'D';
	return 1;
}

FWFunctionSpec (SFVec3d_Functions)[] = {
	{"add", SFVec3d_add, 'W',{1,-1,0,"W"}},
	{"cross", SFVec3d_cross, 'W',{1,-1,0,"W"}},
	{"divide", SFVec3d_divide, 'W',{1,-1,0,"D"}},
	{"dot", SFVec3d_dot, 'D',{1,-1,0,"W"}},
	{"length", SFVec3d_length, 'D',{0,-1,0,NULL}},
	{"multiply", SFVec3d_multiply, 'W',{1,-1,0,"D"}},
	{"negate", SFVec3d_negate, 'W',{0,-1,0,NULL}},
	{"normalize", SFVec3d_normalize, 'W',{0,-1,0,NULL}},
	{"subtract", SFVec3d_subtract, 'W',{1,-1,0,"W"}},
	//{"toString", SFVec3d_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int SFVec3d_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		nr = 1;
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
			fwretval->_numeric =  ptr->c[index];
			//fwretval->_web3dval.anyvrml = (union anyVrml*)&ptr->c[index];
			//fwretval->_web3dval.fieldType = FIELDTYPE_SFFloat;
			break;
		default:
			nr = 0;
		}
	}
	fwretval->itype = 'D';
	return nr;
}
int SFVec3d_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFVec3d_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec3d *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<3;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFVec3d_Properties)[] = {
	{"x", 0, 'F', 0},
	{"y", 1, 'F', 0},
	{"z", 2, 'F', 0},
	{NULL,0,0,0},
};
ArgListType (SFVec3d_ConstructorArgs)[] = {
		{3,0,'T',"DDD"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_SFVec3d	25
FWTYPE SFVec3dType = {
	FIELDTYPE_SFVec3d,
	"SFVec3d",
	sizeof(struct SFVec3d), //sizeof(struct ), 
	SFVec3d_Constructor, //constructor
	SFVec3d_ConstructorArgs, //constructor args
	SFVec3d_Properties, //Properties,
	NULL, //special iterator
	SFVec3d_Getter, //Getter,
	SFVec3d_Setter, //Setter,
	'D',0, //index prop type,readonly
	SFVec3d_Functions, //functions
};


//#define FIELDTYPE_MFVec3d	26
FWTYPE MFVec3dType = {
	FIELDTYPE_MFVec3d,
	"MFVec3d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFDouble	27
FWTYPE SFDoubleType = {
	FIELDTYPE_SFDouble,
	"SFDouble",
	sizeof(double), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	0,0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFDouble	28
FWTYPE MFDoubleType = {
	FIELDTYPE_MFDouble,
	"MFDouble",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'D',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFMatrix3f	29
FWTYPE SFMatrix3fType = {
	FIELDTYPE_SFMatrix3f,
	"SFMatrix3f",
	sizeof(struct SFMatrix3f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'F',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFMatrix3f	30
FWTYPE MFMatrix3fType = {
	FIELDTYPE_MFMatrix3f,
	"MFMatrix3f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFMatrix3d	31
FWTYPE SFMatrix3dType = {
	FIELDTYPE_SFMatrix3d,
	"SFMatrix3d",
	sizeof(struct SFMatrix3d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'D',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFMatrix3d	32
FWTYPE MFMatrix3dType = {
	FIELDTYPE_MFMatrix3d,
	"MFMatrix3d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFMatrix4f	33
FWTYPE SFMatrix4fType = {
	FIELDTYPE_SFMatrix4f,
	"SFMatrix4f",
	sizeof(struct SFMatrix4f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'F',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFMatrix4f	34
FWTYPE MFMatrix4fType = {
	FIELDTYPE_MFMatrix4f,
	"MFMatrix4f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFMatrix4d	35
FWTYPE SFMatrix4dType = {
	FIELDTYPE_SFMatrix4d,
	"SFMatrix4d",
	sizeof(struct SFMatrix4d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'D',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFMatrix4d	36
FWTYPE MFMatrix4dType = {
	FIELDTYPE_MFMatrix4d,
	"MFMatrix4d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec2d	37
FWTYPE SFVec2dType = {
	FIELDTYPE_SFVec2d,
	"SFVec2d",
	sizeof(struct SFVec2d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'D',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec2d	38
FWTYPE MFVec2dType = {
	FIELDTYPE_MFVec2d,
	"MFVec2d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec4f	39
FWTYPE SFVec4fType = {
	FIELDTYPE_SFVec4f,
	"SFVec4f",
	sizeof(struct SFVec4f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'F',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec4f	40
FWTYPE MFVec4fType = {
	FIELDTYPE_MFVec4f,
	"MFVec4f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec4d	41
FWTYPE SFVec4dType = {
	FIELDTYPE_SFVec4d,
	"SFVec4d",
	sizeof(struct SFVec4d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'D',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec4d	42
FWTYPE MFVec4dType = {
	FIELDTYPE_MFVec4d,
	"MFVec4d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};

void initVRMLFields(FWTYPE** typeArray, int *n){
	typeArray[*n] = &SFFloatType; (*n)++;
	typeArray[*n] = &MFFloatType; (*n)++;
	typeArray[*n] = &SFRotationType; (*n)++;
	typeArray[*n] = &MFRotationType; (*n)++;
	typeArray[*n] = &SFVec3fType; (*n)++;
	typeArray[*n] = &MFVec3fType; (*n)++;
	typeArray[*n] = &SFBoolType; (*n)++;
	typeArray[*n] = &MFBoolType; (*n)++;
	typeArray[*n] = &SFInt32Type; (*n)++;
	typeArray[*n] = &MFInt32Type; (*n)++;
	typeArray[*n] = &SFNodeType; (*n)++;
	typeArray[*n] = &MFNodeType; (*n)++;
	typeArray[*n] = &SFColorType; (*n)++;
	typeArray[*n] = &MFColorType; (*n)++;
	typeArray[*n] = &SFColorRGBAType; (*n)++;
	typeArray[*n] = &MFColorRGBAType; (*n)++;
	typeArray[*n] = &SFTimeType; (*n)++;
	typeArray[*n] = &MFTimeType; (*n)++;
	typeArray[*n] = &SFStringType; (*n)++;
	typeArray[*n] = &MFStringType; (*n)++;
	typeArray[*n] = &SFVec2fType; (*n)++;
	typeArray[*n] = &MFVec2fType; (*n)++;
	typeArray[*n] = &SFImageType; (*n)++;
	//typeArray[*n] = &FreeWRLPTRType; (*n)++;
	//typeArray[*n] = &FreeWRLThreadType; (*n)++;
	typeArray[*n] = &SFVec3dType; (*n)++;
	typeArray[*n] = &MFVec3dType; (*n)++;
	typeArray[*n] = &SFDoubleType; (*n)++;
	typeArray[*n] = &MFDoubleType; (*n)++;
	typeArray[*n] = &SFMatrix3fType; (*n)++;
	typeArray[*n] = &MFMatrix3fType; (*n)++;
	typeArray[*n] = &SFMatrix3dType; (*n)++;
	typeArray[*n] = &MFMatrix3dType; (*n)++;
	typeArray[*n] = &SFMatrix4fType; (*n)++;
	typeArray[*n] = &MFMatrix4fType; (*n)++;
	typeArray[*n] = &SFMatrix4dType; (*n)++;
	typeArray[*n] = &MFMatrix4dType; (*n)++;
	typeArray[*n] = &SFVec2dType; (*n)++;
	typeArray[*n] = &MFVec2dType; (*n)++;
	typeArray[*n] = &SFVec4fType; (*n)++;
	typeArray[*n] = &MFVec4fType; (*n)++;
	typeArray[*n] = &SFVec4dType; (*n)++;
	typeArray[*n] = &MFVec4dType; (*n)++;
}

#endif /* ifdef JAVASCRIPT_DUK */
