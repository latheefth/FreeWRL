#include "Eai_C.h"


char *fieldTypeName (char field) {
	switch (field) {
		case X3D_SFUNKNOWN: return "INTERNAL ERROR";
		case X3D_SFBOOL: return "SFBool";
		case X3D_SFCOLOR: return "SFColor";
		case X3D_SFFLOAT: return "SFFloat";
		case X3D_SFTIME: return "SFTime";
		case X3D_SFINT32: return "SFInt32";
		case X3D_SFSTRING: return "SFString";
		case X3D_SFNODE: return "SFNode";
		case X3D_SFROTATION: return "SFRotation";
		case X3D_SFVEC2F: return "SFVec3f";
		case X3D_SFIMAGE: return "SFImage";
		case X3D_MFCOLOR: return "MFColor";
		case X3D_MFFLOAT: return "MFFloat";
		case X3D_MFTIME: return "MFTime";
		case X3D_MFINT32: return "MFInt32";
		case X3D_MFSTRING: return "MFString";
		case X3D_MFNODE: return "MFNode";
		case X3D_MFROTATION: return "MFRotation";
		case X3D_MFVEC2F: return "MFVec2f";
		case X3D_MFVEC3F: return "MFVec3f";
		case X3D_SFVEC3F: return "SFVec3f";
		case X3D_MFCOLORRGBA: return "MFColorRGBA";
		case X3D_SFCOLORRGBA: return "SFColorRGBA";
		case X3D_MFBOOL: return "MFBool";
		case X3D_FREEWRLPTR: return "FreeWRLPTR";
		case X3D_MFVEC3D: return "MFVec3d";
		case X3D_SFVEC2D: return "MFVec2d";
		case X3D_SFVEC3D: return "SFVec3D";
		default :return "OUT OF RANGE";
	}
}

X3D_Node *X3D_newSFVec3f (float a, float b, float c) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFVEC3F;
	retval->X3D_SFVec3f.c[0] = a;
	retval->X3D_SFVec3f.c[1] = b;
	retval->X3D_SFVec3f.c[2] = c;
	return retval;
}

X3D_Node *X3D_newSFColor (float a, float b, float c) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFCOLOR;
	retval->X3D_SFColor.c[0] = a;
	retval->X3D_SFColor.c[1] = b;
	retval->X3D_SFColor.c[2] = c;
	return retval;
}

X3D_Node *X3D_newSFVec2f (float a, float b) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFVEC2F;
	retval->X3D_SFVec2f.c[0] = a;
	retval->X3D_SFVec2f.c[1] = b;
	return retval;
}

X3D_Node *X3D_newSFRotation (float a, float b,float c, float d) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFROTATION;
	retval->X3D_SFRotation.r[0] = a;
	retval->X3D_SFRotation.r[1] = b;
	retval->X3D_SFRotation.r[2] = c;
	retval->X3D_SFRotation.r[3] = d;
	return retval;
}

X3D_Node *X3D_newSFColorRGBA (float a, float b,float c, float d) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFCOLORRGBA;
	retval->X3D_SFRotation.r[0] = a;
	retval->X3D_SFRotation.r[1] = b;
	retval->X3D_SFRotation.r[2] = c;
	retval->X3D_SFRotation.r[3] = d;
	return retval;
}

X3D_Node *X3D_newSFVec3d (double a, double b,double c) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFVEC3D;
	retval->X3D_SFVec3d.c[0] = a;
	retval->X3D_SFVec3d.c[1] = b;
	retval->X3D_SFVec3d.c[2] = c;
	return retval;
}

X3D_Node *X3D_newSFVec2d (double a, double b) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFVEC2D;
	retval->X3D_SFVec3d.c[0] = a;
	retval->X3D_SFVec3d.c[1] = b;
	return retval;
}

X3D_Node *X3D_newSFBool (int a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFBOOL;
	retval->X3D_SFBool.value = a;
	return retval;
}

X3D_Node *X3D_newSFFloat (float a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFFLOAT;
	retval->X3D_SFFloat.value = a;
	return retval;
}

X3D_Node *X3D_newSFTime (double a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFTIME;
	retval->X3D_SFTime.value = a;
	return retval;
}

X3D_Node *X3D_newSFInt32 (int a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = X3D_SFINT32;
	retval->X3D_SFInt32.value = a;
	return retval;
}

X3D_Node *X3D_newSFString(){}
X3D_Node *X3D_newSFNode(){}
X3D_Node *X3D_newSFImage(){}
X3D_Node *X3D_newMFColor(){}
X3D_Node *X3D_newMFFloat(){}
X3D_Node *X3D_newMFTime(){}
X3D_Node *X3D_newMFInt32(){}
X3D_Node *X3D_newMFString(){}
X3D_Node *X3D_newMFNode(){}
X3D_Node *X3D_newMFRotation(){}
X3D_Node *X3D_newMFVec2f(){}
X3D_Node *X3D_newMFVec3f(){}
X3D_Node *X3D_newMFColorRGBA(){}
X3D_Node *X3D_newMFBool(){}
X3D_Node *X3D_newMFVec3d(){}
X3D_Node *X3D_newMFVec2d(){}
