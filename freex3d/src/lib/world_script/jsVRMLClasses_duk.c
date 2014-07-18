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


int SFFloat_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	float *ptr = (float *)fwn;
	fwretval->_numeric =  (double)*(ptr);
	fwretval->itype = 'F';
	//fwretval->_web3dval.native = ptr;
	//fwretval->_web3dval.fieldType = FIELDTYPE_SFFloat;
	//fwretval->itype = 'W';
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
		fwretval->_web3dval.gc = 0;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}
int mf2sf(int itype);
FWTYPE *getFWTYPE(int itype);
char *sfToString(FWType fwt, void *fwn){
	//caller must free / gc the return string
	int i;
	union anyVrml *any = (union anyVrml*)fwn;
	char strbuf[100];
	char *str = NULL;
	switch(fwt->itype){
	case FIELDTYPE_SFBool:
		if(any->sfbool) str = strdup("true");
		else str = strdup("false");
		break;
	case FIELDTYPE_SFInt32:
		sprintf(strbuf,"%d",any->sfint32);
		str = strdup(strbuf);
		break;
	case FIELDTYPE_SFFloat:
		sprintf(strbuf,"%f",any->sffloat);
		str = strdup(strbuf);
		break;
	case FIELDTYPE_SFDouble:
	case FIELDTYPE_SFTime:
		sprintf(strbuf,"%lf",any->sfdouble);
		str = strdup(strbuf);
		break;
	case FIELDTYPE_SFString:{
		str = malloc(strlen(any->sfstring->strptr)+3);
		strcpy(str,"\"");
		str = strcat(str,any->sfstring->strptr);
		str = strcat(str,"\"");
		}
		break;
	default:
	{
		//delegate to SF type toString function
		i = 0;
		while(fwt->Functions[i].name){
			if(!strcmp(fwt->Functions[i].name,"toString")){
				FWval fwpars = NULL;
				FWVAL fwretval;
				fwt->Functions[i].call(fwt,fwn,0,fwpars,&fwretval);
				str = fwretval._string;
				break;
			}
		}
		break;
	}
	}
	return str;
}
char *mfToString(FWType fwt, void * fwn){
	//caller must free / gc the return string
	int i, sftype, len, showType;
	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
	showType = 0; //=1 to see MFColor[], =0 to see []
	len = strlen("[ ");
	if(showType) len += strlen(fwt->name);
	char *str = malloc(len +1);
	str[0] = 0;
	if(showType) strcat(str,fwt->name);
	str = strcat(str,"[ ");
	sftype = mf2sf(fwt->itype);
	FWTYPE *fwtsf = getFWTYPE(sftype);
	for(i=0;i<ptr->n;i++)
	{
		char * sf = sfToString(fwtsf,&ptr->p[i]);
		str = realloc(str,strlen(str)+strlen(sf)+2);
		str = strcat(str,sf);
		str = strcat(str," ");
		free(sf);
	}
	str[strlen(str)-1] = ']';
	return str;
}

int MFW_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	char *str;
	str = mfToString(fwtype,fwn);
	fwretval->_string = str;
	fwretval->itype = 'S';
	return 1;
}

FWFunctionSpec (MFW_Functions)[] = {
	{"toString", MFW_toString, 'S',{0,-1,0,NULL}},
	{0}
};


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
		char *p;
		if(index >= ptr->n){
			//nold = ptr->n;
			ptr->n = index+1;
			//nelen = (int) upper_power_of_two(ptr->n);
			ptr->p = realloc(ptr->p,ptr->n *elen); //need power of 2 if SFNode children
		}
		p = ptr->p + index * elen;
		if(fwval->itype == 'W')
			memcpy(p,fwval->_web3dval.native, elen);
		else{
			float ff;
			switch(fwval->itype){
			case 'B':
				memcpy(p,&fwval->_boolean,elen); break;
			case 'I':
				memcpy(p,&fwval->_integer,elen); break;
			case 'F':
				ff = (float)fwval->_numeric;
				memcpy(p,&ff,elen); break;
			case 'D':
				memcpy(p,&fwval->_numeric,elen); break;
			case 'S':{
				struct Uni_String *uni = newASCIIString(fwval->_string);
				memcpy(p,&uni,elen); break;
				}
			}
		}
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
		if(fwpars[i].itype == 'W' && fwpars[i]._web3dval.fieldType == FIELDTYPE_SFFloat)
			memcpy(p,&fwpars[i]._web3dval.native,lenSF);
		else if(fwpars[i].itype == 'F'){
			float ff = fwpars[i]._numeric;
			memcpy(p,&ff,lenSF);
		}

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
	'F',0, //index prop type,readonly
	MFW_Functions, //functions
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
	{"multVec", SFRotation_multiVec, 'W',{1,-1,0,"W"}}, //freewrl spelling
	{"multiVec", SFRotation_multiVec, 'W',{1,-1,0,"W"}}, //web3d.org spelling
	{"setAxis", SFRotation_setAxis, '0',{1,-1,0,"W"}},
	{"slerp", SFRotation_slerp, 'W',{2,-1,0,"WF"}},
	{"toString", SFRotation_toString, 'S',{0,-1,0,NULL}},
	{0}
};
int SFRotation_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFRotation *ptr = (struct SFRotation *)fwn;
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
	struct SFRotation *ptr = (struct SFRotation *)fwn;
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
	MFW_Functions, //functions
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

int SFVec3f_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3f *ptr = (struct SFVec3f *)fwn;
	char buff[STRING], *str;
	int len;
	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g %.9g",
			ptr->c[0], ptr->c[1], ptr->c[2]);
	len = strlen(buff);
	str = malloc(len+1);  //leak
	strcpy(str,buff);
	fwretval->_string = str;
	fwretval->itype = 'S';
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
	{"toString", SFVec3f_toString, 'S',{0,-1,0,NULL}},
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
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
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


void * MFBool_Constructor(FWType fwtype, int argc, FWval fwpars){
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
		if(fwpars[i].itype == 'W')
			memcpy(p,&fwpars[i]._web3dval.native,lenSF);
		else if(fwpars[i].itype == 'B'){
			memcpy(p,&fwpars[i]._boolean,lenSF);
		}
		p += lenSF;
	}
	return (void *)ptr;
}
ArgListType (MFBool_ConstructorArgs)[] = {
		{0,0,0,"B"},
		{-1,0,0,NULL},
};


//#define FIELDTYPE_MFBool	7
FWTYPE MFBoolType = {
	FIELDTYPE_MFBool,
	"MFBool",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFBool_Constructor, //constructor
	MFBool_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'B',0, //index prop type,readonly
	MFW_Functions, //functions
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


void * MFInt32_Constructor(FWType fwtype, int argc, FWval fwpars){
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
		if(fwpars[i].itype == 'W')
			memcpy(p,&fwpars[i]._web3dval.native,lenSF);
		else if(fwpars[i].itype == 'I')
			memcpy(p,&fwpars[i]._integer,lenSF);
		p += lenSF;
	}
	return (void *)ptr;
}
ArgListType (MFInt32_ConstructorArgs)[] = {
		{0,0,0,"I"},
		{-1,0,0,NULL},
};


//#define FIELDTYPE_MFInt32	9
FWTYPE MFInt32Type = {
	FIELDTYPE_MFInt32,
	"MFInt32",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFInt32_Constructor, //constructor
	MFInt32_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'I',0, //index prop type,readonly
	MFW_Functions, //functions
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
		fwretval->_web3dval.gc = 0;
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
				//ptr = fwpars[0]._web3dval.native; //don't gc
				ptr = malloc(sizeof(void *));
				*ptr = ((union anyVrml*)fwpars[0]._web3dval.native)->sfnode; 

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
	MFW_Functions, //functions
};


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
	MFW_Functions, //functions
};


int SFColorRGBA_getHSV(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//argc should == 0 for getHSV
	struct SFColorRGBA *ptr = (struct SFColorRGBA *)fwn;
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

int SFColorRGBA_setHSV(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//argc should == 3 for setHSV
	struct SFColorRGBA *ptr = (struct SFColorRGBA *)fwn;
	double xp[3];
	/* convert rgb to hsv */
	convertHSVtoRGB((double)fwpars[0]._numeric, (double)fwpars[1]._numeric, (double)fwpars[2]._numeric,&xp[0],&xp[1],&xp[2]);
	ptr->c[0] = (float)xp[0];
	ptr->c[1] = (float)xp[1];
	ptr->c[2] = (float)xp[2];
	ptr->c[3] = 1.0f;
	return 0;
}

FWFunctionSpec (SFColorRGBA_Functions)[] = {
	{"getHSV", SFColor_getHSV, 'W',{0,0,0,NULL}},
	{"setHSV", SFColor_setHSV, 0,{3,-1,'T',"DDD"}},
	{0}
};

int SFColorRGBA_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFColorRGBA *ptr = (struct SFColorRGBA *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		nr = 1;
		switch(index){
		case 0: //r
		case 1: //g
		case 2: //b
		case 3: //a
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
int SFColorRGBA_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFColorRGBA *ptr = (struct SFColorRGBA *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		switch(index){
		case 0: //r
		case 1: //g
		case 2: //b
		case 3: //a
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFColorRGBA_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFColorRGBA *ptr = malloc(fwtype->size_of); //garbage collector please
	if(ic == 4)
	for(int i=0;i<4;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFColorRGBA_Properties)[] = {
	{"r", 0, 'F', 0},
	{"g", 1, 'F', 0},
	{"b", 2, 'F', 0},
	{"a", 3, 'F', 0},
	{NULL,0,0,0},
};

//typedef struct ArgListType {
//	char nfixedArg;
//	char iVarArgStartsAt; //-1 no varargs
//	char fillMissingFixedWithZero; //T/F if F then complain if short
//	char *argtypes; //if varargs, then argtypes[nfixedArg] == type of varArg, and all varargs are assumed the same type
//} ArgListType;
ArgListType (SFColorRGBA_ConstructorArgs)[] = {
		{4,0,'T',"FFFF"},
		{-1,0,0,NULL},
};


//#define FIELDTYPE_SFColorRGBA	14
FWTYPE SFColorRGBAType = {
	FIELDTYPE_SFColorRGBA,
	"SFColorRGBA",
	sizeof(struct SFColorRGBA), //sizeof(struct ), 
	SFColorRGBA_Constructor, //constructor
	SFColorRGBA_ConstructorArgs, //constructor args
	SFColorRGBA_Properties, //Properties,
	NULL, //special iterator
	SFColorRGBA_Getter, //Getter,
	SFColorRGBA_Setter, //Setter,
	'F',0, //index prop type,readonly
	SFColorRGBA_Functions, //functions
};
//#define FIELDTYPE_MFColorRGBA	15
FWTYPE MFColorRGBAType = {
	FIELDTYPE_MFColorRGBA,
	"MFColorRGBA",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
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

void * MFTime_Constructor(FWType fwtype, int argc, FWval fwpars){
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
		if(fwpars[i].itype == 'W')
			memcpy(p,&fwpars[i]._web3dval.native,lenSF);
		else if(fwpars[i].itype == 'D')
			memcpy(p,&fwpars[i]._numeric,lenSF);
		p += lenSF;
	}
	return (void *)ptr;
}
ArgListType (MFTime_ConstructorArgs)[] = {
		{0,0,0,"D"},
		{-1,0,0,NULL},
};


//#define FIELDTYPE_MFTime	17
FWTYPE MFTimeType = {
	FIELDTYPE_MFTime,
	"MFTime",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFTime_Constructor, //constructor
	MFTime_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'D',0, //index prop type,readonly
	MFW_Functions, //functions
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
		else if(fwpars[i].itype == 'S'){
			void *tmp = newASCIIString(fwpars[i]._string);
			memcpy(p,&tmp,lenSF);
			//(*p) = (char *)
		}else if(fwpars[i].itype == 'F' || fwpars[i].itype == 'D'){
			void *tmp;
			char str[100];
			sprintf(str,"%f", fwpars[i]._numeric);
			tmp = newASCIIString(str);
			memcpy(p,&tmp,lenSF);
		}else if(fwpars[i].itype == 'I' ){
			void *tmp;
			char str[100];
			sprintf(str,"%d", fwpars[i]._integer);
			tmp = newASCIIString(str);
			memcpy(p,&tmp,lenSF);
		}else if(fwpars[i].itype == 'B' ){
			void *tmp;
			const char *str = "false";
			if(fwpars[i]._boolean) str = "true";
			tmp = newASCIIString(str);
			memcpy(p,&tmp,lenSF);

		}
		p += lenSF;
	}
	return (void *)ptr;
}
ArgListType (MFString_ConstructorArgs)[] = {
		{0,0,0,"S"},
		{0,0,0,"F"},
		{0,0,0,"D"},
		{0,0,0,"I"},
		{0,0,0,"B"},
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
	'S',0, //index prop type,readonly
	MFW_Functions, //functions
};


/* SFVec2f
Constructor
SFVec2f (numeric x, numeric y) Missing values default to 0.0d+00. 
props
numeric x No First value of the vector 
numeric y No Second value of the vector 
funcs
SFVec2f add(SFVec2f vec) Returns the value of the passed value added, component-wise, to the object. 
SFVec2f divide(numeric n) Returns the value of the object divided by the passed value. 
numeric dot(SFVec2f vec) Returns the dot product of this vector and the passed value. 
numeric length() Returns the geometric length of this vector. 
SFVec2f multiply(numeric n) Returns the value of the object multiplied by the passed value. 
SFVec2f normalize() Returns the object converted to unit length . 
SFVec2f subtract(SFVec2f vec) Returns the value of the passed value subtracted, component-wise, from the object. 
String toString() Returns a String containing the value of x and y encoding using the X3D Classic VRML encoding (see part 2 of ISO/IEC 19776). 
*/

int SFVec2f_add(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	struct SFVec2f *rhs = fwpars[0]._web3dval.native;
	struct SFVec2f *res = malloc(fwtype->size_of);
	vecadd2f(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2f_subtract(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	struct SFVec2f *rhs = fwpars[0]._web3dval.native;
	struct SFVec2f *res = malloc(fwtype->size_of);
	vecdif2f(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2f_divide(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	double rhs = fwpars[0]._numeric;
	if(rhs == 0.0){
		return 0;
	}
	rhs = 1.0/rhs;
	struct SFVec2f *res = malloc(fwtype->size_of);
	vecscale2f(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2f_multiply(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	double rhs = fwpars[0]._numeric;
	struct SFVec2f *res = malloc(fwtype->size_of);
	vecscale2f(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2f_normalize(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	struct SFVec2f *res = malloc(fwtype->size_of);
	vecnormal2f(res->c,ptr->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2f;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFVec2f_length(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	double res;
	res = veclength2f(ptr->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'F';
	return 1;
}
int SFVec2f_dot(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	struct SFVec2f *rhs = fwpars[0]._web3dval.native;
	double res;
	res = vecdot2f(ptr->c,rhs->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'F';
	return 1;
}

int SFVec2f_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	char buff[STRING], *str;
	int len;
	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g",
			ptr->c[0], ptr->c[1]);
	len = strlen(buff);
	str = malloc(len+1);  //leak
	strcpy(str,buff);
	fwretval->_string = str;
	fwretval->itype = 'S';
	return 1;
}
FWFunctionSpec (SFVec2f_Functions)[] = {
	{"add", SFVec2f_add, 'W',{1,-1,0,"W"}},
	{"divide", SFVec2f_divide, 'W',{1,-1,0,"F"}},
	{"dot", SFVec2f_dot, 'F',{1,-1,0,"W"}},
	{"length", SFVec2f_length, 'F',{0,-1,0,NULL}},
	{"multiply", SFVec2f_multiply, 'W',{1,-1,0,"F"}},
	{"normalize", SFVec2f_normalize, 'W',{0,-1,0,NULL}},
	{"subtract", SFVec2f_subtract, 'W',{1,-1,0,"W"}},
	{"toString", SFVec2f_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int SFVec2f_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 2){
		nr = 1;
		switch(index){
		case 0: //x
		case 1: //y
			fwretval->_numeric =  ptr->c[index];
			//fwretval->_web3dval.anyvrml = (union anyVrml*)&ptr->c[index];
			//fwretval->_web3dval.fieldType = FIELFTYPE_SFFloat;
			break;
		default:
			nr = 0;
		}
	}
	fwretval->itype = 'F';
	return nr;
}
int SFVec2f_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFVec2f *ptr = (struct SFVec2f *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 2){
		switch(index){
		case 0: //x
		case 1: //y
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFVec2f_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec2f *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<2;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFVec2f_Properties)[] = {
	{"x", 0, 'F', 0},
	{"y", 1, 'F', 0},
	{NULL,0,0,0},
};
ArgListType (SFVec2f_ConstructorArgs)[] = {
		{2,0,'T',"FF"},
		{-1,0,0,NULL},
};

//#define FIELDTYPE_SFVec2f	20
FWTYPE SFVec2fType = {
	FIELDTYPE_SFVec2f,
	"SFVec2f",
	sizeof(struct SFVec2f), //sizeof(struct ), 
	SFVec2f_Constructor, //constructor
	SFVec2f_ConstructorArgs, //constructor args
	SFVec2f_Properties, //Properties,
	NULL, //special iterator
	SFVec2f_Getter, //Getter,
	SFVec2f_Setter, //Setter,
	'F',0, //index prop type,readonly
	SFVec2f_Functions, //functions
};

//#define FIELDTYPE_MFVec2f	21
FWTYPE MFVec2fType = {
	FIELDTYPE_MFVec2f,
	"MFVec2f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
};

/* SFImage
http://www.web3d.org/files/specifications/19777-1/V3.0/Part1/functions.html#SFImage
constr
SFImage (numeric x, numeric y, numeric comp, MFInt32 array) x is the x-dimension of the image. y is the y-dimension of the image. comp is the number of components of the image (1 for greyscale, 2 for greyscale+alpha, 3 for rgb, 4 for rgb+alpha). Array contains the x * y values for the pixels of the image. The format of each pixel is an SFImage as in the PixelTexture node (see part 1 of ISO/IEC 19775).  
props
numeric width No Width dimension of the image in pixels 
numeric height No Height dimension of the image in pixels 
numeric comp No Number of components of the image
1.greyscale or alpha
2.greyscale + alpha
3.rgb
4.rgb + alpha
MFInt32 array No Returns a String containing the  value of x, y, comp and array encoded using the Classic VRML encoding (see part 2 of ISO/IEC 19776). 
funcs
String toString() Returns a String containing the  value of x, y, comp and array encoded using the Classic VRML encoding (see part 2 of ISO/IEC 19776). 
*/

int SFImage_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	char *str;
	FWType mfint32type = getFWTYPE(FIELDTYPE_MFInt32);
	str = mfToString(mfint32type, fwn);
	fwretval->_string = str;
	fwretval->itype = 'S';
	return 1;
}

FWFunctionSpec (SFImage_Functions)[] = {
	{"toString", SFImage_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int SFImage_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct Multi_Int32 *ptr = (struct Multi_Int32 *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		nr = 1;
		switch(index){
		case 0: //width
		case 1: //height
		case 2: //comp
		fwretval->_integer =  ptr->p[index];
		fwretval->itype = 'I';
		break;

		case 3: //array
		fwretval->_web3dval.native = ptr; //hope they don't go image.array[0] = infinity; which will overwrite width. same for height, comp
		fwretval->_web3dval.fieldType = FIELDTYPE_MFInt32;
		fwretval->_web3dval.gc = 0;
		fwretval->itype = 'W';
		break;
		default:
		nr = 0;
		}
	}
	return nr;
}
int SFImage_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct Multi_Int32 *ptr = (struct Multi_Int32*)fwn;
	int *p;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		switch(index){
			case 0: //width
			case 1: //height
			case 2: //comp
			ptr->p[index] = fwval->_integer;
			p = ptr->p;
			if(ptr->n < (p[0] * p[1] * p[2]) ){
				//resize
				ptr->n = (p[0] * p[1] * p[2]);
				ptr->p = realloc(ptr->p,ptr->n);
			}
			break;

			case 3: //array
			if(fwval->itype == 'W' && fwval->_web3dval.fieldType == FIELDTYPE_MFInt32 ){
				int width,height,comp;
				int i,j, ncopy;
				struct Multi_Int32 *im = fwval->_web3dval.native;
				ncopy = min(ptr->n,im->n);
				//don't write over width,height,comp
				memcpy(&ptr->p[3],&im->p[3],(ncopy-3)*sizeof(int));
				//for(i=3;i<ncopy;i++)
				//	ptr->p[i] = im->p[i];
			}
			break;
			default:
				break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFImage_Constructor(FWType fwtype, int ic, FWval fwpars){
	//around freewrl, SFImage is stored as a MFIn32, with n = 3 x width x height, 
	//and the first (int,int,int) pixel sacrificed to hold (width,height,comp)
	int width, height, comp;
	struct Multi_Int32 *ptr = malloc(fwtype->size_of); //garbage collector please

	width = fwpars[0]._integer;
	height = fwpars[1]._integer;
	comp = fwpars[2]._integer;
	ptr->n = 3 * width * height;
	ptr->p = malloc(ptr->n * sizeof(int)); //garbage collector please
	if(fwpars[3].itype == 'W' && fwpars[3]._web3dval.fieldType == FIELDTYPE_MFInt32){
		//the incoming MFInt32 pixel values are one pixel per Int32, so we need to expand to 3 ints
		int i,j, ncopy;
		struct Multi_Int32 *im = fwpars[3]._web3dval.native;
		ncopy = min(ptr->n,im->n);
		for(i=0;i<ncopy;i++)
			ptr->p[i] = im->p[i];
	}
	//first 3 ints are sacrificed
	ptr->p[0] = width;
	ptr->p[1] = height;
	ptr->p[2] = comp;
	return (void *)ptr;
}

FWPropertySpec (SFImage_Properties)[] = {
	{"width", 0, 'I', 0},
	{"height", 1, 'I', 0},
	{"comp", 2, 'I', 0},
	{"array", 3, 'W', 0},
	{NULL,0,0,0},
};
ArgListType (SFImage_ConstructorArgs)[] = {
		{4,3,'T',"IIIW"},
		{-1,0,0,NULL},
};


//#define FIELDTYPE_SFImage	22
FWTYPE SFImageType = {
	FIELDTYPE_SFImage,
	"SFImage",
	sizeof(struct Multi_Int32), 
	SFImage_Constructor, //constructor
	SFImage_ConstructorArgs, //constructor args
	SFImage_Properties, //Properties,
	NULL, //special iterator
	SFImage_Getter, //Getter,
	SFImage_Setter, //Setter,
	0,0, //index prop type,readonly
	SFImage_Functions, //functions
};

#define FIELDTYPE_MFImage	43 
FWTYPE MFImageType = {
	FIELDTYPE_MFImage,
	"MFImage",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
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

int SFVec3d_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	char buff[STRING], *str;
	int len;
	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g %.9g",
			ptr->c[0], ptr->c[1], ptr->c[2]);
	len = strlen(buff);
	str = malloc(len+1);  //leak
	strcpy(str,buff);
	fwretval->_string = str;
	fwretval->itype = 'S';
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
	{"toString", SFVec3d_toString, 'S',{0,-1,0,NULL}},
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
	{"x", 0, 'D', 0},
	{"y", 1, 'D', 0},
	{"z", 2, 'D', 0},
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
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
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

void * MFDouble_Constructor(FWType fwtype, int argc, FWval fwpars){
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
		if(fwpars[i].itype == 'W')
			memcpy(p,&fwpars[i]._web3dval.native,lenSF);
		else if(fwpars[i].itype == 'D')
			memcpy(p,&fwpars[i]._numeric,lenSF);
		p += lenSF;
	}
	return (void *)ptr;
}
ArgListType (MFDouble_ConstructorArgs)[] = {
		{0,0,0,"D"},
		{-1,0,0,NULL},
};

//#define FIELDTYPE_MFDouble	28
FWTYPE MFDoubleType = {
	FIELDTYPE_MFDouble,
	"MFDouble",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFDouble_Constructor, //constructor
	MFDouble_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'D',0, //index prop type,readonly
	MFW_Functions, //functions
};

// http://www.web3d.org/files/specifications/19777-1/V3.0/Part1/functions.html#Matrix3
/* Matrix3
constr
X3DMatrix3 (numeric f11, numeric f12, numeric f13,
  numeric f21, numeric f22, numeric f23,
  numeric f31, numeric f32, numeric f33) The creation function shall initialize the array using zero or more SFVec3-valued expressions passed as parameters. 
props
- row major single index
funcs
void setTransform(SFVec2f translation, SFVec3f rotation, SFVec2f scale, SFVec3f scaleOrientation, SFVec2f center)
	Sets the Matrix to the passed values. Any of the rightmost parameters may be omitted. The function has zero to five parameters. For example, specifying zero parameters results in an identity matrix while specifying one parameter results in a translation and specifying two parameters results in a translation and a rotation. Any unspecified parameter is set to its default as specified for the Transform node. Values are applied to the matrix in the same order as the matrix field calculations for the Transform node. 
void getTransform(SFVec2f translation, SFVec3f rotation, SFVec2f scale) 
	Decomposes the Matrix and returns the components in the passed translation, rotation, and scale objects. The types of these passed objects is the same as the first three arguments to setTransform. If any passed object is not sent, or if the null object is sent for any value, that value is not returned. Any projection or shear information in the matrix is ignored. 
Matrix3 inverse()
	Returns a Matrix whose value is the inverse of this object. 
Matrix3 transpose()
	Returns a Matrix whose value is the transpose of this object. 
Matrix3 multLeft(Matrix3)
	Returns a Matrix whose value is the object multiplied by the passed matrix on the left. 
Matrix3 multRight(Matrix3)
	Returns a Matrix whose value is the object multiplied by the passed matrix on the right. 
SFVec2f multVecMatrix(SFVec2f vec)
	Returns an SFVec3f whose value is the object multiplied by the passed row vector. 
SFVec2f multMatrixVec(SFVec2f vec)
	Returns an SFVec3f whose value is the object multiplied by the passed column vector. 
String toString() Returns a String containing the matrix contents encoded using the X3D Classic VRML encoding (see part 2 of ISO/IEC 19776). 
I assume they mean homogenous transform, 2D
[x']   [x  y  1] X [c*sx  s*sy  px]   [x]   //px, py are 2D perspectives
[y'] =             [-s*sx c*sy  py] X [y]
[w ]               [tx    ty     1]   [1]  //tx,ty are 2D translations
x" = x'/w
y" = y'/w
dug9 complaint about Matrix3.getTransform, .setTransform july 2014:
A 2D planar rotation can be represented by a scalar angle. I have no idea where the angle
is in the SFVec3f. I could guess the angle is in [4] in the SFRotation, and assume
the axis part is 0,0,1. 
I gather the reason they pass in complex types for rotations in setTransform is 
a) so null can be used to signal no-value (but 0 for rotation would do the same)
b) because they want them returned in getTransform and to get something returned via
function args you have to pass in a pointer type, not an ecma primitive value. 
Primitive values can't be returned through function args only through return vals. 
They could have:
1) broken the get into getScale, getRotation, getTranslation, and then the getRotation could return
an ecma numeric primitive, or 
2) numeric getTransform(scale,translation) with the numeric return val being the rotation, or
3) defined an SFFloat complex type and passed it as a pointer object
I will implment july 2014 the rotations as scalar/primitive/numerics and do #2, which doesn't comply with specs

*/


int X3DMatrix3_setTransform(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/group.html#Transform
	// P' = T * C * R * SR * S * -SR * -C * P
	int i,j;
	float angle, scaleangle;
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFVec2f *translation = fwpars[0]._web3dval.native;
	struct SFVec3f *rotation = NULL;
	if(fwpars[1].itype == 'W' && fwpars[1]._web3dval.fieldType == FIELDTYPE_SFVec3f){
		rotation = fwpars[1]._web3dval.native;
		angle = rotation->c[0]; //your guess is as good as mine what they meant
	}
	if(fwpars[1].itype == 'F')
		angle = (float)fwpars[1]._numeric;
	struct SFVec2f *scale = fwpars[2]._web3dval.native;
	struct SFVec3f *scaleOrientation = NULL;
	if(fwpars[3].itype == 'W' && fwpars[3]._web3dval.fieldType == FIELDTYPE_SFVec3f){
		scaleOrientation = fwpars[3]._web3dval.native;
		scaleangle = scaleOrientation->c[0]; //your guess is as good as mine what they meant
	}
	if(fwpars[3].itype == 'F')
		scaleangle = (float)fwpars[3]._numeric;
	struct SFVec2f *center = fwpars[4]._web3dval.native;
	float *matrix[3], m2[9], *mat[3];
	for(i=0;i<3;i++){
		matrix[0] = &ptr->c[i*3];
		mat[i] = &m2[i*3];
	}
	//initialize to Identity
	matidentity3f(matrix[0]);

	//-C
	if(center){
		matidentity3f(mat[0]);
		veccopy2f(mat[2],center->c);
		vecscale2f(mat[2],mat[3],-1.0f);
		matmultiply3f(matrix[0],mat[0],matrix[0]);
	}
	//-SR
	if(scaleangle != 0.0f){
		matidentity3f(mat[0]);
		mat[0][0] =  mat[1][1] = cos(-scaleangle);
		mat[0][1] =  mat[1][0] = sin(-scaleangle);
		mat[0][1] = -mat[0][1];
		matmultiply3f(matrix[0],mat[0],matrix[0]);
	}
	//S
	if(scale){
		matidentity4f(mat[0]);
		for(i=0;i<3;i++)
			vecmult2f(mat[i],mat[i],scale->c);
		matmultiply3f(matrix[0],mat[0],matrix[0]);
	}
	//SR
	if(scaleangle != 0.0f){
		matidentity3f(mat[0]);
		mat[0][0] =  mat[1][1] = cos(scaleangle);
		mat[0][1] =  mat[1][0] = sin(scaleangle);
		mat[0][1] = -mat[0][1];
		matmultiply3f(matrix[0],mat[0],matrix[0]);
	}
	//R
	if(angle != 0.0f){
		matidentity3f(mat[0]);
		mat[0][0] =  mat[1][1] = cos(angle);
		mat[0][1] =  mat[1][0] = sin(angle);
		mat[0][1] = -mat[0][1];
		matmultiply3f(matrix[0],mat[0],matrix[0]);
	}
	//C
	if(center){
		matidentity3f(mat[0]);
		veccopy2f(mat[2],center->c);
		matmultiply3f(matrix[0],mat[0],matrix[0]);
	}
	//T
	if(translation){
		matidentity3f(mat[0]);
		veccopy2f(mat[2],translation->c);
		matmultiply3f(matrix[0],mat[0],matrix[0]);
	}

	return 0;
}

int X3DMatrix3_getTransform(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//void getTransform(SFVec2f translation, SFVec3f rotation, SFVec2f scale) 
	float angle = 0.0f;
	int i;
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFVec2f *translation = fwpars[0]._web3dval.native;
	struct SFVec3f *rotation = NULL;
	if(fwpars[1].itype == 'W' && fwpars[1]._web3dval.fieldType == FIELDTYPE_SFVec3f){
		rotation = fwpars[1]._web3dval.native;
		angle = rotation->c[3]; //your guess is as good as mine
	}else if(fwpars[1].itype == 'F'){
		angle = (float)fwpars[1]._numeric;
	}
	struct SFVec2f *scale = fwpars[2]._web3dval.native;
    float *matrix[3], retscale[2];
	for(i=0;i<3;i++)
		matrix[i] = &ptr->c[i*3];

	//get row scales
	for(i=0;i<2;i++)
		retscale[i] = (float)sqrt(vecdot3f(matrix[i],matrix[i]));

	if (translation) {
		veccopy2f(translation->c,matrix[2]);
	}

	/* rotation */
	if (1) {
		/* apply length to each row to normalize upperleft 3x3 to rotations and shears*/
		float m2[9], ff;
		for(i=0;i<2;i++){
			ff = retscale[i];
			if(ff != 0.0f) ff = 1/ff;
				vecscale3f(&m2[i*3],matrix[i],ff);
		}
		angle = atan2(m2[1],m2[2]);
		/* now copy the values over */
		if(rotation) 
			rotation->c[3] = angle;
	}

	/* scale */
	if (scale) {
		veccopy2f(scale->c,retscale);
	}

	fwretval->itype = 'F';
	fwretval->_numeric = angle;
	return 1;
}

int X3DMatrix3_inverse(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFMatrix3f *ret = malloc(sizeof(struct SFMatrix3f));

	matrix3x3_inverse_float(ptr->c, ret->c);

	fwretval->_pointer.native = ret;
	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix3;
	fwretval->_pointer.gc = 'T';
	fwretval->itype = 'P';
	return 1;
}
int X3DMatrix3_transpose(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFMatrix3f *ret = malloc(sizeof(struct SFMatrix3f));

	mattranspose3f(ret->c, ptr->c);

	fwretval->_pointer.native = ret;
	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix3;
	fwretval->_pointer.gc = 'T';
	fwretval->itype = 'P';

	return 1;
}
int X3DMatrix3_multLeft(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFMatrix3f *lhs = (struct SFMatrix3f *)fwpars[0]._web3dval.native;
	struct SFMatrix3f *ret = malloc(sizeof(struct SFMatrix3f));

	matmultiply3f(ret->c,  lhs->c , ptr->c);

	fwretval->_pointer.native = ret;
	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix3;
	fwretval->itype = 'P';
	fwretval->_pointer.gc = 'T';
	return 1;
}
int X3DMatrix3_multRight(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFMatrix3f *rhs = (struct SFMatrix3f *)fwpars[0]._web3dval.native;
	struct SFMatrix3f *ret = malloc(sizeof(struct SFMatrix3f));
	fwretval->_pointer.native = ret;

	matmultiply3f(ret->c,  ptr->c, rhs->c);

	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix3;
	fwretval->_pointer.gc = 'T';
	fwretval->itype = 'P';
	return 1;
}
int X3DMatrix3_multVecMatrix(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFVec2f *rhs = (struct SFVec2f *)fwpars[0]._web3dval.native;
	struct SFVec2f *ret = malloc(sizeof(struct SFVec2f));
	float a3[3], r3[3];
	veccopy2f(a3,rhs->c);
	a3[2] = 1.0f;
	vecmultmat3f(r3, a3, ptr->c);
	if(r3[2] != 0.0f){
		float wi = 1.0f/r3[2];
		vecscale2f(ret->c,r3,wi);
	}else{
		veccopy2f(ret->c,r3);
	}

	fwretval->_web3dval.native = ret;
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2f;
	fwretval->_web3dval.gc = 'T';
	fwretval->itype = 'W';
	return 1;
}
int X3DMatrix3_multMatrixVec(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	struct SFVec2f *rhs = (struct SFVec2f *)fwpars[0]._web3dval.native;
	struct SFVec2f *ret = malloc(sizeof(struct SFVec2f));
	float a3[3], r3[3];
	veccopy2f(a3,rhs->c);
	a3[2] = 1.0f;
	matmultvec3f(r3, ptr->c,a3);
	if(r3[2] != 0.0f){
		float wi = 1.0f/r3[2];
		vecscale2f(ret->c,r3,wi);
	}else{
		veccopy2f(ret->c,r3);
	}

	fwretval->_web3dval.native = ret;
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2f;
	fwretval->_web3dval.gc = 'T';
	fwretval->itype = 'W';
	return 1;
}

int X3DMatrix3_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	char *str, *r;
	int i;
	FWType sfvec3ftype = getFWTYPE(FIELDTYPE_SFVec3f);

	str = NULL;
	for(i=0;i<3;i++){
		r = sfToString(sfvec3ftype,&ptr->c[i*3]);
		str = realloc(str,strlen(str)+strlen(r)+2);
		str = strcat(str,r);
	}
	fwretval->_string = str;
	fwretval->itype = 'S';
	return 1;
}

FWFunctionSpec (X3DMatrix3_Functions)[] = {
	{"setTransform", X3DMatrix3_setTransform, 0,{5,-1,0,"WWWWW"}},
	{"getTransform", X3DMatrix3_getTransform, 'P',{1,-1,0,"W"}},
	{"inverse", X3DMatrix3_inverse, 'P',{0,-1,0,NULL}},
	{"transpose", X3DMatrix3_transpose, 'P',{0,-1,0,NULL}},
	{"multLeft", X3DMatrix3_multLeft, 'P',{1,-1,0,"P"}},
	{"multRight", X3DMatrix3_multRight, 'P',{1,-1,0,"P"}},
	{"multVecMatrix", X3DMatrix3_multVecMatrix, 'W',{1,-1,0,"W"}},
	{"multMatrixVec", X3DMatrix3_multMatrixVec, 'W',{1,-1,0,"W"}},
	{"toString", X3DMatrix3_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int X3DMatrix3_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 9){
		nr = 1;
		fwretval->_numeric =  ptr->c[index];
	}
	fwretval->itype = 'F';
	return nr;
}
int X3DMatrix3_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 9){
		if(fwval->itype = 'F'){
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			return TRUE;
		}
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * X3DMatrix3_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec3d *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<3;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

ArgListType (X3DMatrix3_ConstructorArgs)[] = {
		{9,0,'T',"FFFFFFFFF"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_SFMatrix3f	29
FWTYPE X3DMatrix3Type = {
	AUXTYPE_X3DMatrix3,
	"X3DMatrix3",
	sizeof(struct SFMatrix3f), //sizeof(struct ), 
	X3DMatrix3_Constructor, //constructor
	X3DMatrix3_ConstructorArgs, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	X3DMatrix3_Getter, //Getter,
	X3DMatrix3_Setter, //Setter,
	'F',0, //index prop type,readonly
	X3DMatrix3_Functions, //functions
};


/* 
Matrix4
http://www.web3d.org/files/specifications/19777-1/V3.0/Part1/functions.html#Matrix4
"The translation elements are in the fourth row. For example, x3dMatrixObjectName[3][0] is the X offset"
- I assume its a 4x4 homogenous transform matrix
[x']   [x y z 1] X [   scale      px]   where px,py,pz are perspectives ie pz = 1/focal-length
[y'] =             [   and        py] 
[z']               [   rot        pz] 
[w ]               [tx   ty   tz   1]   and tx,ty,tz are translations
x" = x'/w
y" = y'/w
z" = z'/w

constr
X3DMatrix4 (numeric f11, numeric f12, numeric f13, numeric f14,
  numeric f21, numeric f22, numeric f23, numeric f24,
  numeric f31, numeric f32, numeric f33, numeric f34,
  numeric f41, numeric f42, numeric f43, numeric f44) The creation function shall initialize the array using zero or more SFVec3-valued expressions passed as parameters. 
props
array-style indexing row-major order
funcs
void setTransform(SFVec3f translation, SFRotation rotation, SFVec3f scale, SFRotation scaleOrientation, SFVec3f center) 
	Sets the Matrix to the passed values. Any of the rightmost parameters may be omitted. The function has zero to five parameters. For example, specifying zero parameters results in an identity matrix while specifying one parameter results in a translation and specifying two parameters results in a translation and a rotation. Any unspecified parameter is set to its default as specified for the Transform node. Values are applied to the matrix in the same order as the matrix field calculations for the Transform node. 
void getTransform(SFVec3f translation, SFRotation rotation, SFVec3f scale) 
	Decomposes the Matrix and returns the components in the passed translation, rotation, and scale objects. The types of these passed objects is the same as the first three arguments to setTransform. If any passed object is not sent, or if the null object is sent for any value, that value is not returned. Any projection or shear information in the matrix is ignored.  
Matrix4 inverse()      Returns a Matrix whose value is the inverse of this object. 
Matrix4 transpose()    Returns a Matrix whose value is the transpose of this object. 
Matrix4 multLeft(Matrix4 matrix)   Returns a Matrix whose value is the object multiplied by the passed matrix on the left. 
Matrix4 multRight(Matrix4 matrix)    Returns a Matrix whose value is the object multiplied by the passed matrix on the right. 
SFVec3f multVecMatrix(SFVec3f vec)    Returns an SFVec3f whose value is the object multiplied by the passed row vector. 
SFVec3f multMatrixVec(SFVec3f vec)    Returns an SFVec3f whose value is the object multiplied by the passed column vector. 
String toString()    Returns a String containing the matrix contents encoded using the X3D Classic VRML encoding (see part 2 of ISO/IEC 19776). 

*/

int X3DMatrix4_setTransform(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//void setTransform(SFVec3f translation, SFRotation rotation, SFVec3f scale, SFRotation scaleOrientation, SFVec3f center) 
	// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/group.html#Transform
	// P' = T * C * R * SR * S * -SR * -C * P
	int i,j;
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFVec3f *translation = fwpars[0]._web3dval.native;
	struct SFRotation *rotation = fwpars[1]._web3dval.native;
	struct SFVec3f *scale = fwpars[2]._web3dval.native;
	struct SFRotation *scaleOrientation = fwpars[3]._web3dval.native;
	struct SFVec3f *center = fwpars[4]._web3dval.native;
	//set up some [][] helpers for clarity
	float *matrix[4], *mat[4], m2[16];
	for(i=0;i<4;i++){
		matrix[i] = &ptr->c[i*4]; //our current matrix3
		mat[i] = &m2[i*4]; //scratch matrix
	}
	//initialize to Identity
	matidentity4f(matrix[0]);
	//-C
	if(center){
		matidentity4f(mat[0]);
		veccopy3f(mat[3],center->c);
		vecscale3f(mat[3],mat[3],-1.0f);
		matmultiply4f(matrix[0],mat[0],matrix[0]);
	}
	//-SR
	if(scaleOrientation){
		scaleOrientation->c[3] = -scaleOrientation->c[3];
		matidentity4f(mat[0]);
		for(i=0;i<3;i++)
			axisangle_rotate3f(mat[i], mat[i], scaleOrientation->c);
		matmultiply4f(matrix[0],mat[0],matrix[0]);
		scaleOrientation->c[3] = -scaleOrientation->c[3];
	}
	//S
	if(scale){
		matidentity4f(mat[0]);
		for(i=0;i<4;i++)
			vecmult3f(mat[i],mat[i],scale->c);
		matmultiply4f(matrix[0],mat[0],matrix[0]);
	}
	//SR
	if(scaleOrientation){
		matidentity4f(mat[0]);
		for(i=0;i<3;i++)
			axisangle_rotate3f(mat[i], mat[i], scaleOrientation->c);
		matmultiply4f(matrix[0],mat[0],matrix[0]);
	}
	//R
	if(rotation){
		matidentity4f(mat[0]);
		for(i=0;i<3;i++)
			axisangle_rotate3f(mat[i], mat[i], rotation->c);
		matmultiply4f(matrix[0],mat[0],matrix[0]);

	}
	//C
	if(center){
		matidentity4f(mat[0]);
		veccopy3f(mat[3],center->c);
		matmultiply4f(matrix[0],mat[0],matrix[0]);
	}
	//T
	if(translation){
		matidentity4f(mat[0]);
		veccopy3f(mat[3],translation->c);
		matmultiply4f(matrix[0],mat[0],matrix[0]);
	}

	return 0;
}

int X3DMatrix4_getTransform(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//void getTransform(SFVec3f translation, SFRotation rotation, SFVec3f scale) 
	int i;
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFVec3f *translation = fwpars[0]._web3dval.native;
	struct SFRotation *rotation = fwpars[1]._web3dval.native;
	struct SFVec3f *scale = fwpars[2]._web3dval.native;
    float *matrix[4], retscale[3];
	double matrixd[16];
    	Quaternion quat;
    	double qu[4];
	double r0[4], r1[4], r2[4];
	for(i=0;i<4;i++)
		matrix[i] = &ptr->c[i*4];

	//get row scales
	for(i=0;i<3;i++)
		retscale[i] = (float)sqrt(vecdot4f(matrix[i],matrix[i]));

	if (translation) {
		veccopy3f(translation->c,matrix[3]);
	}

	/* rotation */
	if (rotation) {
		/* apply length to each row to normalize upperleft 3x3 to rotations and shears*/
		float m2[16], ff;
		for(i=0;i<3;i++){
			ff = retscale[i];
			if(ff != 0.0f) ff = 1/ff;
				vecscale4f(&m2[i*4],matrix[i],ff);
		}
		/* convert the matrix to a quaternion */
		for(i=0;i<16;i++) matrixd[i] = (double) m2[i];
		matrix_to_quaternion (&quat, matrixd);
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("quaternion %f %f %f %f\n",quat.x,quat.y,quat.z,quat.w);
		#endif

		/* convert the quaternion to a VRML rotation */
		quaternion_to_vrmlrot(&quat, &qu[0],&qu[1],&qu[2],&qu[3]);

		/* now copy the values over */
		for (i=0; i<4; i++) 
			rotation->c[i] = (float) qu[i];
	}

	/* scale */
	if (scale) {
		veccopy3f(scale->c,retscale);
	}

	return 0;
}

int X3DMatrix4_inverse(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//Matrix4 inverse()
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFMatrix4f *ret = malloc(sizeof(struct SFMatrix4f));

	matinverse4f(ret->c,ptr->c);

	fwretval->_pointer.native = ret;
	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix4;
	fwretval->_pointer.gc = 'T';
	fwretval->itype = 'P';
	return 1;
}
int X3DMatrix4_transpose(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//Matrix4 transpose()    Returns a Matrix whose value is the transpose of this object. 
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFMatrix4f *ret = malloc(sizeof(struct SFMatrix4f));

	mattranspose4f(ret->c, ptr->c);

	fwretval->_pointer.native = ret;
	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix4;
	fwretval->_pointer.gc = 'T';
	fwretval->itype = 'P';

	return 1;
}
int X3DMatrix4_multLeft(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//Matrix4 multLeft(Matrix4 matrix)   Returns a Matrix whose value is the object multiplied by the passed matrix on the left. 

	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFMatrix4f *rhs = (struct SFMatrix4f *)fwpars[0]._web3dval.native;
	struct SFMatrix4f *ret = malloc(sizeof(struct SFMatrix4f));

	matmultiply4f(ptr->c,rhs->c,ptr->c);

	fwretval->_pointer.native = ret;
	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix4;
	fwretval->_pointer.gc = 'T';
	fwretval->itype = 'P';
	return 1;
}
int X3DMatrix4_multRight(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//Matrix4 multRight(Matrix4 matrix)    Returns a Matrix whose value is the object multiplied by the passed matrix on the right. 
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFMatrix4f *rhs = (struct SFMatrix4f *)fwpars[0]._web3dval.native;
	struct SFMatrix4f *ret = malloc(sizeof(struct SFMatrix4f));
	fwretval->_pointer.native = ret;

	matmultiply4f(ptr->c,ptr->c,rhs->c);

	fwretval->_pointer.fieldType = AUXTYPE_X3DMatrix4;
	fwretval->_pointer.gc = 'T';
	fwretval->itype = 'P';
	return 1;
}
int X3DMatrix4_multVecMatrix(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//SFVec3f multVecMatrix(SFVec3f vec)    Returns an SFVec3f whose value is the object multiplied by the passed row vector. 
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFVec3f *rhs = (struct SFVec3f *)fwpars[0]._web3dval.native;
	struct SFVec3f *ret = malloc(sizeof(struct SFVec3f));
	float a4[4], r4[4];
	veccopy3f(a4,rhs->c);
	a4[3] = 1.0f;
	vecmultmat4f(r4, a4, ptr->c);
	if(r4[3] != 0.0f){
		float wi = 1.0f/r4[3];
		vecscale3f(ret->c,r4,wi);
	}else{
		veccopy3f(ret->c,r4);
	}

	fwretval->_web3dval.native = ret;
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T';
	fwretval->itype = 'W';
	return 1;
}
int X3DMatrix4_multMatrixVec(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//SFVec3f multMatrixVec(SFVec3f vec)    Returns an SFVec3f whose value is the object multiplied by the passed column vector. 
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	struct SFVec3f *rhs = (struct SFVec3f *)fwpars[0]._web3dval.native;
	struct SFVec3f *ret = malloc(sizeof(struct SFVec3f));
//float* matmultvec4f(float* r4, float *mat4, float* a4 );
	float a4[4], r4[4];
	veccopy3f(a4,rhs->c);
	a4[3] = 1.0f;
	matmultvec4f(r4, ptr->c, a4 );
	if(r4[3] != 0.0f){
		float wi = 1.0f/r4[3];
		vecscale3f(ret->c,r4,wi);
	}else{
		veccopy3f(ret->c,r4);
	}

	fwretval->_web3dval.native = ret;
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3f;
	fwretval->_web3dval.gc = 'T';
	fwretval->itype = 'W';
	return 1;
}

int X3DMatrix4_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFMatrix4f *ptr = (struct SFMatrix4f *)fwn;
	char *str, *r;
	int i;
	FWType sfvec4ftype = getFWTYPE(FIELDTYPE_SFVec4f);

	str = NULL;
	for(i=0;i<4;i++){
		r = sfToString(sfvec4ftype,&ptr->c[i*4]);
		str = realloc(str,strlen(str)+strlen(r)+2);
		str = strcat(str,r);
	}
	fwretval->_string = str;
	fwretval->itype = 'S';
	return 1;
}


FWFunctionSpec (X3DMatrix4_Functions)[] = {
	{"setTransform", X3DMatrix4_setTransform, 0,{5,-1,0,"WWWWW"}},
	{"getTransform", X3DMatrix4_getTransform, 'P',{1,-1,0,"W"}},
	{"inverse", X3DMatrix4_inverse, 'P',{0,-1,0,NULL}},
	{"transpose", X3DMatrix4_transpose, 'P',{0,-1,0,NULL}},
	{"multLeft", X3DMatrix4_multLeft, 'P',{1,-1,0,"P"}},
	{"multRight", X3DMatrix4_multRight, 'P',{1,-1,0,"P"}},
	{"multVecMatrix", X3DMatrix4_multVecMatrix, 'W',{1,-1,0,"W"}},
	{"multMatrixVec", X3DMatrix4_multMatrixVec, 'W',{1,-1,0,"W"}},
	{"toString", X3DMatrix4_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int X3DMatrix4_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 9){
		nr = 1;
		fwretval->_numeric =  ptr->c[index];
	}
	fwretval->itype = 'F';
	return nr;
}
int X3DMatrix4_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFMatrix3f *ptr = (struct SFMatrix3f *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 9){
		if(fwval->itype = 'F'){
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			return TRUE;
		}
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * X3DMatrix4_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec3d *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<3;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

ArgListType (X3DMatrix4_ConstructorArgs)[] = {
		{9,0,'T',"FFFFFFFFF"},
		{-1,0,0,NULL},
};

FWTYPE X3DMatrix4Type = {
	AUXTYPE_X3DMatrix4,
	"X3DMatrix4",
	sizeof(struct SFMatrix3f), //sizeof(struct ), 
	X3DMatrix4_Constructor, //constructor
	X3DMatrix4_ConstructorArgs, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	X3DMatrix4_Getter, //Getter,
	X3DMatrix4_Setter, //Setter,
	'F',0, //index prop type,readonly
	X3DMatrix4_Functions, //functions
};


// http://www.web3d.org/files/specifications/19777-1/V3.0/Part1/functions.html#SFVec2d
/* SFVec2d
Constructor
SFVec2d (numeric x, numeric y) Missing values default to 0.0d+00. 
props
numeric x No First value of the vector 
numeric y No Second value of the vector 
funcs
SFVec2d add(SFVec2d vec) Returns the value of the passed value added, component-wise, to the object. 
SFVec2d divide(numeric n) Returns the value of the object divided by the passed value. 
numeric dot(SFVec2d vec) Returns the dot product of this vector and the passed value. 
numeric length() Returns the geometric length of this vector. 
SFVec2d multiply(numeric n) Returns the value of the object multiplied by the passed value. 
SFVec2d normalize() Returns the object converted to unit length . 
SFVec2d subtract(SFVec2d vec) Returns the value of the passed value subtracted, component-wise, from the object. 
String toString() Returns a String containing the value of x and y encoding using the X3D Classic VRML encoding (see part 2 of ISO/IEC 19776). 
*/

int SFVec2d_add(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	struct SFVec2d *rhs = fwpars[0]._web3dval.native;
	struct SFVec2d *res = malloc(fwtype->size_of);
	vecadd2d(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2d_subtract(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	struct SFVec2d *rhs = fwpars[0]._web3dval.native;
	struct SFVec2d *res = malloc(fwtype->size_of);
	vecdif2d(res->c,ptr->c,rhs->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2d_divide(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	double rhs = fwpars[0]._numeric;
	if(rhs == 0.0){
		return 0;
	}
	rhs = 1.0/rhs;
	struct SFVec2d *res = malloc(fwtype->size_of);
	vecscale2d(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2d_multiply(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	double rhs = fwpars[0]._numeric;
	struct SFVec2d *res = malloc(fwtype->size_of);
	vecscale2d(res->c,ptr->c,rhs);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}
int SFVec2d_normalize(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	struct SFVec2d *res = malloc(fwtype->size_of);
	vecnormal2d(res->c,ptr->c);
	fwretval->_web3dval.native = res; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec2d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}

int SFVec2d_length(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	double res;
	res = veclength2d(ptr->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'D';
	return 1;
}
int SFVec2d_dot(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	struct SFVec2d *rhs = fwpars[0]._web3dval.native;
	double res;
	res = vecdot2d(ptr->c,rhs->c);
	fwretval->_numeric = res; 
	fwretval->itype = 'D';
	return 1;
}
int SFVec2d_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	char buff[STRING], *str;
	int len;
	memset(buff, 0, STRING);
	sprintf(buff, "%.9g %.9g",
			ptr->c[0], ptr->c[1]);
	len = strlen(buff);
	str = malloc(len+1);  //leak
	strcpy(str,buff);
	fwretval->_string = str;
	fwretval->itype = 'S';
	return 1;
}
FWFunctionSpec (SFVec2d_Functions)[] = {
	{"add", SFVec2d_add, 'W',{1,-1,0,"W"}},
	{"divide", SFVec2d_divide, 'W',{1,-1,0,"D"}},
	{"dot", SFVec2d_dot, 'D',{1,-1,0,"W"}},
	{"length", SFVec2d_length, 'D',{0,-1,0,NULL}},
	{"multiply", SFVec2d_multiply, 'W',{1,-1,0,"D"}},
	{"normalize", SFVec2d_normalize, 'W',{0,-1,0,NULL}},
	{"subtract", SFVec2d_subtract, 'W',{1,-1,0,"W"}},
	{"toString", SFVec2d_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int SFVec2d_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 2){
		nr = 1;
		switch(index){
		case 0: //x
		case 1: //y
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
int SFVec2d_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFVec2d *ptr = (struct SFVec2d *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 2){
		switch(index){
		case 0: //x
		case 1: //y
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFVec2d_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec2d *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<3;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFVec2d_Properties)[] = {
	{"x", 0, 'D', 0},
	{"y", 1, 'D', 0},
	{NULL,0,0,0},
};
ArgListType (SFVec2d_ConstructorArgs)[] = {
		{2,0,'T',"DD"},
		{-1,0,0,NULL},
};

//#define FIELDTYPE_SFVec2d	37
FWTYPE SFVec2dType = {
	FIELDTYPE_SFVec2d,
	"SFVec2d",
	sizeof(struct SFVec2d), //sizeof(struct ), 
	SFVec2d_Constructor, //constructor
	SFVec2d_ConstructorArgs, //constructor args
	SFVec2d_Properties, //Properties,
	NULL, //special iterator
	SFVec2d_Getter, //Getter,
	SFVec2d_Setter, //Setter,
	'D',0, //index prop type,readonly
	SFVec2d_Functions, //functions
};

//#define FIELDTYPE_MFVec2d	38
FWTYPE MFVec2dType = {
	FIELDTYPE_MFVec2d,
	"MFVec2d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
};

int SFVec4f_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec4f *ptr = (struct SFVec4f *)fwn;
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

FWFunctionSpec (SFVec4f_Functions)[] = {
	{"toString", SFVec4f_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int SFVec4f_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFVec4f *ptr = (struct SFVec4f *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		nr = 1;
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
		case 3: //t
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
int SFVec4f_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFVec4f *ptr = (struct SFVec4f *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
		case 3: //t
			ptr->c[index] = (float)fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFVec4f_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec4f *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<4;i++)
		ptr->c[i] =  (float)fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFVec4f_Properties)[] = {
	{"x", 0, 'F', 0},
	{"y", 1, 'F', 0},
	{"z", 2, 'F', 0},
	{"w", 3, 'F', 0},
	{NULL,0,0,0},
};
ArgListType (SFVec4f_ConstructorArgs)[] = {
		{4,0,'T',"FFFF"},
		{-1,0,0,NULL},
};


//#define FIELDTYPE_SFVec4f	39
FWTYPE SFVec4fType = {
	FIELDTYPE_SFVec4f,
	"SFVec4f",
	sizeof(struct SFVec4f), //sizeof(struct ), 
	SFVec4f_Constructor, //constructor
	SFVec4f_ConstructorArgs, //constructor args
	SFVec4f_Properties, //Properties,
	NULL, //special iterator
	SFVec4f_Getter, //Getter,
	SFVec4f_Setter, //Setter,
	'F',0, //index prop type,readonly
	SFVec4f_Functions, //functions
};

//#define FIELDTYPE_MFVec4f	40
FWTYPE MFVec4fType = {
	FIELDTYPE_MFVec4f,
	"MFVec4f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
};

int SFVec4d_toString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	struct SFVec4d *ptr = (struct SFVec4d *)fwn;
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
FWFunctionSpec (SFVec4d_Functions)[] = {
	{"toString", SFVec4d_toString, 'S',{0,-1,0,NULL}},
	{0}
};

int SFVec4d_Getter(FWType fwt, int index, void * fwn, FWval fwretval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		nr = 1;
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
		case 3: //t
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
int SFVec4d_Setter(FWType fwt, int index, void * fwn, FWval fwval){
	struct SFVec3d *ptr = (struct SFVec3d *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 4){
		switch(index){
		case 0: //x
		case 1: //y
		case 2: //z
		case 3: //t
			ptr->c[index] = fwval->_numeric; //fwval->_web3dval.anyvrml->sffloat; 
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFVec4d_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFVec3d *ptr = malloc(fwtype->size_of); //garbage collector please
	for(int i=0;i<4;i++)
		ptr->c[i] =  fwpars[i]._numeric; //fwpars[i]._web3dval.anyvrml->sffloat; //
	return (void *)ptr;
}

FWPropertySpec (SFVec4d_Properties)[] = {
	{"x", 0, 'D', 0},
	{"y", 1, 'D', 0},
	{"z", 2, 'D', 0},
	{"w", 3, 'D', 0},
	{NULL,0,0,0},
};
ArgListType (SFVec4d_ConstructorArgs)[] = {
		{3,0,'T',"DDDD"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_SFVec4d	41
FWTYPE SFVec4dType = {
	FIELDTYPE_SFVec4d,
	"SFVec4d",
	sizeof(struct SFVec4d), //sizeof(struct ), 
	SFVec4d_Constructor, //constructor
	SFVec4d_ConstructorArgs, //constructor args
	SFVec4d_Properties, //Properties,
	NULL, //special iterator
	SFVec4d_Getter, //Getter,
	SFVec4d_Setter, //Setter,
	'D',0, //index prop type,readonly
	SFVec4d_Functions, //functions
};

//#define FIELDTYPE_MFVec4d	42
FWTYPE MFVec4dType = {
	FIELDTYPE_MFVec4d,
	"MFVec4d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFW_Getter, //Getter,
	MFW_Setter, //Setter,
	'W',0, //index prop type,readonly
	MFW_Functions, //functions
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
	//typeArray[*n] = &SFMatrix3fType; (*n)++;
	//typeArray[*n] = &MFMatrix3fType; (*n)++;
	//typeArray[*n] = &SFMatrix3dType; (*n)++;
	//typeArray[*n] = &MFMatrix3dType; (*n)++;
	//typeArray[*n] = &SFMatrix4fType; (*n)++;
	//typeArray[*n] = &MFMatrix4fType; (*n)++;
	//typeArray[*n] = &SFMatrix4dType; (*n)++;
	//typeArray[*n] = &MFMatrix4dType; (*n)++;
	typeArray[*n] = &SFVec2dType; (*n)++;
	typeArray[*n] = &MFVec2dType; (*n)++;
	typeArray[*n] = &SFVec4fType; (*n)++;
	typeArray[*n] = &MFVec4fType; (*n)++;
	typeArray[*n] = &SFVec4dType; (*n)++;
	typeArray[*n] = &MFVec4dType; (*n)++;
}

#endif /* ifdef JAVASCRIPT_DUK */
