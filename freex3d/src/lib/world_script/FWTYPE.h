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
    char		type; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	char		isReadOnly; //T/F
} FWPropertySpec;

typedef struct ArgListType {
	char nfixedArg;
	char iVarArgStartsAt; //-1 no varargs
	char fillMissingFixedWithZero; //T/F if F then complain if short
	char *argtypes; //if varargs, then argtypes[nfixedArg] == type of varArg, and all varargs are assumed the same type
} ArgListType;

typedef int (* FWFunction)();
typedef struct FWFunctionSpec {
    const char		*name;
    FWFunction		call;
	char			retType; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d F-jsFunc P=ptr
	struct ArgListType arglist;
} FWFunctionSpec;

typedef struct FWTYPE{
	char *name;
	//int index; //index into an array of FWTYPES
	int size_of; //for mallocing in constructor
	struct ArgListType *Constructors;
	FWPropertySpec *Properties;
	FWFunction *Getter;
	FWFunction *Setter;
	int takesIndexer; //getter can take in integer index ie MF[33]
	FWFunctionSpec *Functions;
} FWTYPE;
typedef struct FWTYPE *FWType;

//wrapper around *native with a few extras 
typedef struct WEB3DNATIVE {
	int fieldType;      //type of vrml field (use FIELDTYPE_SFNode for nodes, else ie FIELDTYPE_SFVec3f)
	void *native;		//pointer to anyVrml
	int *valueChanged; 	//pointer to valueChanged != NULL if this FWNATIVe is a reference to a Script->Field
} *Web3dNative;

//our version of a variant, except in C types and our union anyVrml
typedef struct FWVAL{
	char itype; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr
	union {
		int _null;
		double _numeric;
		int _integer;
		char* _string;
		void* _pointer;
		Web3dNative _web3dval;
		void* _jsobject; //placeholder for js function callback objects
	};
} *FWval;
FWval FWvalsNew(int argc);


typedef int (* FWConstructor)(FWType fwtype, Web3dNative fwn, int argc, FWval *fwpars);
typedef int (* FWFunction)(FWType fwtype, Web3dNative fwn, int argc, FWval *fwpars, FWval *fwretval);
typedef int (* FWGet)(FWType fwtype, Web3dNative fwn, FWval *fwretval);
typedef int (* FWSet)(FWType fwtype, Web3dNative fwn, FWval *fwsetval);
//typedef void (* FWFinalizer)(FWType fwtype, FWNative fwn);


#endif /* __FWTYPE_H__ */
