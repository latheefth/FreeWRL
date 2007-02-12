#include "Eai_C.h"

X3D_Node *X3D_newSFVec3f (float a, float b, float c) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFVec3f;
	retval->X3D_SFVec3f.c[0] = a;
	retval->X3D_SFVec3f.c[1] = b;
	retval->X3D_SFVec3f.c[2] = c;
	return retval;
}

X3D_Node *X3D_newSFColor (float a, float b, float c) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFColor;
	retval->X3D_SFColor.c[0] = a;
	retval->X3D_SFColor.c[1] = b;
	retval->X3D_SFColor.c[2] = c;
	return retval;
}

X3D_Node *X3D_newSFVec2f (float a, float b) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFVec2f;
	retval->X3D_SFVec2f.c[0] = a;
	retval->X3D_SFVec2f.c[1] = b;
	return retval;
}

X3D_Node *X3D_newSFRotation (float a, float b,float c, float d) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFRotation;
	retval->X3D_SFRotation.r[0] = a;
	retval->X3D_SFRotation.r[1] = b;
	retval->X3D_SFRotation.r[2] = c;
	retval->X3D_SFRotation.r[3] = d;
	return retval;
}

X3D_Node *X3D_newSFColorRGBA (float a, float b,float c, float d) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFColorRGBA;
	retval->X3D_SFRotation.r[0] = a;
	retval->X3D_SFRotation.r[1] = b;
	retval->X3D_SFRotation.r[2] = c;
	retval->X3D_SFRotation.r[3] = d;
	return retval;
}

X3D_Node *X3D_newSFBool (int a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFBool;
	retval->X3D_SFBool.value = a;
	return retval;
}

X3D_Node *X3D_newSFFloat (float a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFFloat;
	retval->X3D_SFFloat.value = a;
	return retval;
}

X3D_Node *X3D_newSFTime (double a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFTime;
	retval->X3D_SFTime.value = a;
	return retval;
}

X3D_Node *X3D_newSFInt32 (int a) {
	X3D_Node *retval;
	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFInt32;
	retval->X3D_SFInt32.value = a;
	return retval;
}

X3D_Node *X3D_newSFString(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newSFNode(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newSFImage(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFColor(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFFloat(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFTime(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFInt32(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFString(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFNode(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFRotation(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFVec2f(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFVec3f(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFColorRGBA(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFBool(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFVec3d(){printf ("New node not implemented yet for this type\n");return NULL;}
X3D_Node *X3D_newMFVec2d(){printf ("New node not implemented yet for this type\n");return NULL;}
