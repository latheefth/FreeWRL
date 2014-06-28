/*
=INSERT_TEMPLATE_HERE=

$Id: FWTYPE.h,v 1.9 2010/02/26 19:34:44 sdumoulin Exp $

Local include for world_script directory.

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


#ifndef __FWTYPE_H__
#define __FWTYPE_H__

typedef struct FWPropertySpec {
    const char	*name; //NULL means index int: SFVec3f[0], MF[i]
    char		index; //stable property index for switch/casing instead of strcmp on name
    char		type; //0 = ival, 1=dval, 2=cval, 3=fwnval 4=vrmlval 5=ptr
	char		isReadOnly; 
} FWPropertySpec;

typedef struct ArgListType {
	char nfixedArg;
	char iVarArgStartsAt; //-1 no varargs
	char fillMissingFixedWithZero; //T/F if F then complain if short
	char argtypes[24]; //if varargs, then argtypes[nfixedArg] == type of varArg, and all varargs are assumed the same type
} ArgListType;

typedef int (* FWFunction)();
typedef struct FWFunctionSpec {
    const char		*name;
    FWFunction		call;
	int				retType;
	struct ArgListType arglist;
} FWFunctionSpec;

typedef struct FWTYPE{
	char *name;
	//int index; //index into an array of FWTYPES
	int size_of; //for mallocing in constructor
	struct ArgListType *constructors;
	FWPropertySpec *Properties;
	FWFunctionSpec *Functions;
} FWTYPE;
typedef struct FWTYPE *FWType;

//wrapper around *native with a few extras 
typedef struct FWNATIVE {
	int fwItype;        //type of vrml field
	void *native;		//pointer to anyVrml
	int *valueChanged; 	//pointer to valueChanged != NULL if this FWNATIVe is a reference to a Script->Field
} *FWNative;

//our version of a variant, except in C types and our union anyVrml
typedef struct FWVAL{
	int itype; //0 =bool, 1=num, 2=string, 3=ptr 4=object.fwnval 5=object.callback
	union {
		int ival;
		double dval;
		char* cval;
		void* ptr;
		FWNative fwnval;
		//could / should be something for javascript callback objects?
		//should there be a null/nil/undefined?
	};
} *FWval;
FWval FWvalsNew(int argc);


typedef int (* FWConstructor)(FWType fwtype, FWNative fwn, int argc, FWval *fwpars);
typedef int (* FWFunction)(FWType fwtype, FWNative fwn, int argc, FWval *fwpars, FWval *fwretval);
typedef int (* FWGet)(FWType fwtype, FWNative fwn, FWval *fwretval);
typedef int (* FWSet)(FWType fwtype, FWNative fwn, FWval *fwsetval);
//typedef void (* FWFinalizer)(FWType fwtype, FWNative fwn);


#endif /* __FWTYPE_H__ */
