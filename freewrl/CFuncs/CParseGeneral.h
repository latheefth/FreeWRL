/* General header for VRML-parser (lexer/parser) */

#ifndef CPARSEGENERAL_H
#define CPARSEGENERAL_H

#include <stdio.h>

#include "headers.h"

/* Typedefs for VRML-types. */
typedef int	vrmlBoolT;
typedef struct SFColor	vrmlColorT;
typedef struct SFColorRGBA	vrmlColorRGBAT;
typedef float	vrmlFloatT;
typedef int32_t	vrmlInt32T;
typedef struct Multi_Int32*	vrmlImageT;
typedef struct X3D_Node*	vrmlNodeT;
typedef struct SFRotation	vrmlRotationT;
typedef SV*	vrmlStringT;
typedef double	vrmlTimeT;
typedef struct SFVec2f	vrmlVec2fT;
typedef struct SFColor	vrmlVec3fT;

/* This is an union to hold every vrml-type */
union anyVrml
{
 vrmlBoolT sfbool;
 vrmlColorT sfcolor;
 vrmlColorRGBAT sfcolorRGBA;
 vrmlFloatT sffloat;
 vrmlInt32T sfint32;
 vrmlImageT sfimage;
 vrmlNodeT sfnode;
 vrmlRotationT sfrotation;
 vrmlStringT sfstring;
 vrmlTimeT sftime;
 vrmlVec2fT sfvec2f;
 vrmlVec3fT sfvec3f;
};

#define parseError(msg) \
 (ConsoleMessage("Parse error:  " msg "\n"), fprintf(stderr, msg "\n")) \

#endif /* Once-check */
