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
    char		type; //0 = null, F=Float D=Double I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr Z=flexiString(SFString,MFString[0] or ecmaString)
	char		readOnly; //T/F
} FWPropertySpec;

typedef struct ArgListType {
	char nfixedArg;
	char iVarArgStartsAt; //-1 no varargs
	char fillMissingFixedWithZero; //T/F if F then complain if short
	char *argtypes; //if varargs, then argtypes[nfixedArg] == type of varArg, and all varargs are assumed the same type
} ArgListType;

typedef int (* FWFunction)();
typedef int (* FWGet)();
typedef int (* FWSet)();
typedef int (* FWIterator)();
typedef void *(* FWConstructor)();

typedef struct FWFunctionSpec {
    const char		*name;
    FWFunction		call;
	char			retType; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d F-jsFunc P=ptr
	struct ArgListType arglist;
} FWFunctionSpec;

typedef struct FWTYPE{
	int itype; //AUXTYPE_ or FIELDTYPE_
	char ctype;  //what it maps to in fwval.itype: B,I,F,D,S,W,P
	char *name;
	//int index; //index into an array of FWTYPES
	int size_of; //for mallocing in constructor
	FWConstructor Constructor;
	struct ArgListType *ConstructorArgs;
	FWPropertySpec *Properties;
	FWIterator iterator; //if NULL and Properties, then will use generic function to has, else if !Properties and iterator, then will iterator in has
	FWGet Getter; //Q should I have a virtual Getter that takes a char* key and does its own lookup?
	FWSet Setter;
	char takesIndexer; char indexerReadOnly;//getter can take in integer index ie MF[33]. put 0 or FALSE for no, else put the type the property takes/gives ie 'W' 'S' 'I' 'N' 'B' 'P'
	FWFunctionSpec *Functions;
} FWTYPE, *FWType;

//wrapper around *native with a few extras 
typedef struct WEB3DNATIVE {
	int fieldType;      //type of vrml field (use FIELDTYPE_SFNode for nodes, else ie FIELDTYPE_SFVec3f)
	union {
	void *native;		//pointer to auxtype - you would assign to this
	union anyVrml *anyvrml;		//pointer to anyVrml - you can use this during debugging to inspect the native you assigned
	};
	int *valueChanged; 	//pointer to valueChanged != NULL if this FWNATIVe is a reference to a Script->Field
	char gc; //'T' or 1 if you malloced the pointer and want the engine to free() when it garbage collects the related obj
} FWPointer;// *Web3dNative, 

#define AUXTYPE_X3DConstants 1001
#define AUXTYPE_X3DBrowser 1002
#define AUXTYPE_ComponentInfoArray 1010
#define AUXTYPE_ProfileInfoArray 1011
#define AUXTYPE_ComponentInfo 1012
#define AUXTYPE_ProfileInfo 1013
#define AUXTYPE_X3DRoute 1014
#define AUXTYPE_X3DRouteArray 1015
#define AUXTYPE_X3DScene 1016
#define AUXTYPE_X3DExecutionContext 1017
#define AUXTYPE_X3DMatrix3 1018
#define AUXTYPE_X3DMatrix4 1019
//typedef struct FWPointer{
//	int auxtype; //gets asigned to fwItype, so cget can switch (instead of using sfnode and declaring a _nodeType in your auxiliary type struct)
//	void *ptr;
//} * FwPointer;
//our version of a variant, except in C types and our union anyVrml
typedef struct FWVAL{
	char itype; //0 = null, F=Float D=Double I=Integer B=Boolean S=String, Z=flexiString (MF or S) W=Object-web3d O-js Object P=ptr
	union {
		//union anyScalar{
		int _null;
		double _numeric;
		int _integer;
		int _boolean;
		const char* _string;
		//}
		FWPointer _pointer;
		FWPointer _web3dval;
		void* _jsobject; //placeholder for js function callback objects
	};
} FWVAL, *FWval;
FWval FWvalsNew(int argc);

typedef void * (* FWConstructor)(FWType fwtype, int ic, FWval fwpars);
typedef int (* FWFunction)(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval);
typedef int (* FWGet)(FWType fwtype, int index, void * fwn, FWval fwretval);
typedef int (* FWSet)(FWType fwtype, int index, void * fwn, FWval fwsetval);
typedef int (* FWIterator)(int index, FWType fwt, FWPointer *pointer, char **name, int *lastProp, int *jndex, char *type, char *readOnly);
//typedef void (* FWFinalizer)(FWType fwtype, FWNative fwn);
struct Multi_Any {int n; char *p;}; //should be same size as {int n, double *p} or {int n, struct X3D_Node **p} - an int and a pointer



#endif /* __FWTYPE_H__ */
