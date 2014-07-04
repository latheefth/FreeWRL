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


/********************************************************/
/*							*/
/* Second part - SF classes				*/
/*							*/
/********************************************************/

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
	union anyVrml *any = malloc(sizeof(union anyVrml)); //garbage collector please
	memcpy(any->sfvec3d.c,xp,sizeof(double)*3);
	fwretval->_web3dval.native = any; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->itype = 'W';
	return 1;
}





//#define FIELDTYPE_SFFloat	0
FWTYPE SFFloatType = {
	FIELDTYPE_SFFloat,
	"SFFloat",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFFloat	1
FWTYPE MFFloatType = {
	FIELDTYPE_MFFloat,
	"MFFloat",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFRotation	2
FWTYPE SFRotationType = {
	FIELDTYPE_SFRotation,
	"SFRotation",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFRotation	3
FWTYPE MFRotationType = {
	FIELDTYPE_MFRotation,
	"MFRotation",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFVec3f	4
FWTYPE SFVec3fType = {
	FIELDTYPE_SFVec3f,
	"SFVec3f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFVec3f	5
FWTYPE MFVec3fType = {
	FIELDTYPE_MFVec3f,
	"MFVec3f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFBool	6
FWTYPE SFBoolType = {
	FIELDTYPE_SFBool,
	"SFBool",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFBool	7
FWTYPE MFBoolType = {
	FIELDTYPE_MFBool,
	"MFBool",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFInt32	8
FWTYPE SFInt32Type = {
	FIELDTYPE_SFInt32,
	"SFInt32",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFInt32	9
FWTYPE MFInt32Type = {
	FIELDTYPE_MFInt32,
	"MFInt32",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFNode	10
FWTYPE SFNodeType = {
	FIELDTYPE_SFNode,
	"SFNode",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFNode	11
FWTYPE MFNodeType = {
	FIELDTYPE_MFNode,
	"MFNode",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFColor	12
FWTYPE SFColorType = {
	FIELDTYPE_SFColor,
	"SFColor",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFColor	13
FWTYPE MFColorType = {
	FIELDTYPE_MFColor,
	"MFColor",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFColorRGBA	14
FWTYPE SFColorRGBAType = {
	FIELDTYPE_SFColorRGBA,
	"SFColorRGBA",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFColorRGBA	15
FWTYPE MFColorRGBAType = {
	FIELDTYPE_MFColorRGBA,
	"MFColorRGBA",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFTime	16
FWTYPE SFTimeType = {
	FIELDTYPE_SFTime,
	"SFTime",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFTime	17
FWTYPE MFTimeType = {
	FIELDTYPE_MFTime,
	"MFTime",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFString	18
FWTYPE SFStringType = {
	FIELDTYPE_SFString,
	"SFString",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFString	19
FWTYPE MFStringType = {
	FIELDTYPE_MFString,
	"MFString",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFVec2f	20
FWTYPE SFVec2fType = {
	FIELDTYPE_SFVec2f,
	"SFVec2f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec2f	21
FWTYPE MFVec2fType = {
	FIELDTYPE_MFVec2f,
	"MFVec2f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFImage	22
FWTYPE SFImageType = {
	FIELDTYPE_SFImage,
	"SFImage",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_FreeWRLPTR	23
//#define FIELDTYPE_FreeWRLThread	24
//#define FIELDTYPE_SFVec3d	25
FWTYPE SFVec3dType = {
	FIELDTYPE_SFVec3d,
	"SFVec3d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec3d	26
FWTYPE MFVec3dType = {
	FIELDTYPE_MFVec3d,
	"MFVec3d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFDouble	27
FWTYPE SFDoubleType = {
	FIELDTYPE_SFDouble,
	"SFDouble",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFDouble	28
FWTYPE MFDoubleType = {
	FIELDTYPE_MFDouble,
	"MFDouble",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFMatrix3f	29
FWTYPE SFMatrix3fType = {
	FIELDTYPE_SFMatrix3f,
	"SFMatrix3f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFMatrix3f	30
FWTYPE MFMatrix3fType = {
	FIELDTYPE_MFMatrix3f,
	"MFMatrix3f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFMatrix3d	31
FWTYPE SFMatrix3dType = {
	FIELDTYPE_SFMatrix3d,
	"SFMatrix3d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFMatrix3d	32
FWTYPE MFMatrix3dType = {
	FIELDTYPE_MFMatrix3d,
	"MFMatrix3d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFMatrix4f	33
FWTYPE SFMatrix4fType = {
	FIELDTYPE_SFMatrix4f,
	"SFMatrix4f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFMatrix4f	34
FWTYPE MFMatrix4fType = {
	FIELDTYPE_MFMatrix4f,
	"MFMatrix4f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFMatrix4d	35
FWTYPE SFMatrix4dType = {
	FIELDTYPE_SFMatrix4d,
	"SFMatrix4d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFMatrix4d	36
FWTYPE MFMatrix4dType = {
	FIELDTYPE_MFMatrix4d,
	"MFMatrix4d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec2d	37
FWTYPE SFVec2dType = {
	FIELDTYPE_SFVec2d,
	"SFVec2d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec2d	38
FWTYPE MFVec2dType = {
	FIELDTYPE_MFVec2d,
	"MFVec2d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec4f	39
FWTYPE SFVec4fType = {
	FIELDTYPE_SFVec4f,
	"SFVec4f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec4f	40
FWTYPE MFVec4fType = {
	FIELDTYPE_MFVec4f,
	"MFVec4f",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec4d	41
FWTYPE SFVec4dType = {
	FIELDTYPE_SFVec4d,
	"SFVec4d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec4d	42
FWTYPE MFVec4dType = {
	FIELDTYPE_MFVec4d,
	"MFVec4d",
	0, //sizeof(struct ), 
	NULL, //constructor
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
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
