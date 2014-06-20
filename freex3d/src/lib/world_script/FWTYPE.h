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

//#define LARGESTRING 2048
#define STRING 512
//#define SMALLSTRING 128

typedef struct FWPropertySpec {
    const char            *name;
    int                   index;
    int                   type; //0 = ival, 1=dval, 2=cval, 3=fwnval 4=vrmlval 5=ptr
} FWPropertySpec;

//typedef struct ArgType {
//	char *chartype;
//	int valtype;
//} ArgType;
//typedef struct ConstructorScheme {
//	int argc;
//	ArgType *argTypes;
//} constructorScheme;
//typedef struct ConstructorSchemes {
//	int n;
//	constructorScheme *schemes;
//} ConstructorSchemes;

typedef struct FWConstructorSpec{
	int argc;
	int argtypes[5]; //or int * for indefinite number?
} FWConstructorSpec;

typedef int (* FWFunction)();
typedef struct FWFunctionSpec {
    const char		*name;
    FWFunction		call;
	int				retType;
    int				nargs;
    int				argtypes; //list of parameter types - should be array, but for now I'll assume homogenous arg types
} FWFunctionSpec;

typedef struct FWTYPE{
	char *name;
	int index; //index into an array of FWTYPES
	FWPropertySpec *Properties;
	int nprops;
	FWFunctionSpec *Functions; //int *(* Functions)(); //void *Functions;
	int nfuncs;
	int (* constr)(); //void *constr;
	//ConstructorSchemes constructorSchemes;
	FWConstructorSpec *constructorSchemes;
	int nschemes;
	int (* getter)();
	int (* setter)();
} FWTYPE;
typedef struct FWTYPE *FWType;





//wrapper around *native with a few extras 
// (replaces old *NodeNative types with a generic one)
typedef struct FWNATIVE {
	FWType fwtype;        //declarative-boilerplate struct
	void *native;		//pointer to freewrl type such as struct SFColor
	int valueChanged; 	//instead of putting in a class-specific SFColorNative, it's in a generic fwNative
	void *nativeParent; 	//to get to any parent valueChanged
//OR	int *parentValueChanged;  //pointer directly to parent valueChanged
	int wasMalloced; 	//(versus wrap-static/other-owned) if wasmalloced, free native in finalize
} *FWNative;

//our version of a variant, except in C types and our union anyVrml
typedef struct FWVAL{
	int itype; //0 = ival, 1=dval, 2=cval, 3=fwnval 4=vrmlval 5=ptr
	union {
		int ival;
		double dval;
		char* cval;
		FWNative fwnval;
		union anyVrml* vrmlval; //not needed here but analogous 'variant'
		void* ptr;
	};
} *FWval;


//typedef struct *FWNative;
//FWFunction used for both function and constructor:
typedef int (* FWFunction)(FWType fwtype, FWNative fwn, int argc, FWval *fwpars, FWval *fwretval);
typedef int (* FWGetter)(FWType fwtype, FWNative fwn, FWval *fwretval);
typedef int (* FWSetter)(FWType fwtype, FWNative fwn, FWval *fwsetval);
typedef void (* FWFinalizer)(FWType fwtype, FWNative fwn);

FWNative *FWNativeNew();
FWval FWvalsNew(int argc);


#endif /* __FWTYPE_H__ */
