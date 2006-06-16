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
typedef SV*	vrmlImageT;
typedef struct X3D_Node*	vrmlNodeT;
typedef struct SFRotation	vrmlRotationT;
typedef SV*	vrmlStringT;
typedef double	vrmlTimeT;
typedef struct SFVec2f	vrmlVec2fT;
typedef struct SFColor	vrmlVec3fT;

#define parseError(msg) \
 fputs("Parse error:  " msg "\n", stderr)

#endif /* Once-check */
