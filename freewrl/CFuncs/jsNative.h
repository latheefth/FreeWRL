/*
 * Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License
 * (file COPYING in the distribution) for conditions of use and
 * redistribution, EXCEPT on the files which belong under the
 * Mozilla public license.
 *
 * $Id$
 *
 *
 */

#ifndef __jsNative_h__
#define __jsNative_h__

#include "Structs.h" /* FreeWRL C structs */

typedef struct _BrowserNative {
	/* int magic; does this really do anything ??? */
	/* and, this really does nothing SV *sv_js; */
} BrowserNative;

typedef struct _SFNodeNative {
	int touched;
	uintptr_t *handle;
	char *X3DString;
} SFNodeNative;

typedef struct _SFRotationNative {
	int touched;
	struct SFRotation v;
} SFRotationNative;

typedef struct _SFVec2fNative {
	int touched;
	struct SFVec2f v;
} SFVec2fNative;

typedef struct _SFVec3fNative {
	int touched;
	struct SFColor v;
} SFVec3fNative;

typedef struct _SFImageNative {
	int touched;
#if FALSE
	/* struct SFImage v; */
#endif
} SFImageNative;

typedef struct _SFColorNative {
	int touched;
	struct SFColor v;
} SFColorNative;

typedef struct _SFColorRGBANative {
	int touched;
	struct SFColorRGBA v;
} SFColorRGBANative;


/*
 * Adds additional (touchable) property to instance of a native
 * type.
 */
extern JSBool
addGlobalECMANativeProperty(void *cx,
							void *glob,
							char *name);

extern JSBool
addGlobalAssignProperty(void *cx,
						void *glob,
						char *name,
						char *str);

extern JSBool
addSFNodeProperty(void *cx,
				  void *glob,
				  char *nodeName,
				  char *name,
				  char *str);

extern void *
SFNodeNativeNew(void);

extern void
SFNodeNativeDelete(void *p);

extern JSBool
SFNodeNativeAssign(void *top, void *fromp);

extern void *
SFRotationNativeNew(void);

extern void
SFRotationNativeDelete(void *p);

extern void
SFRotationNativeAssign(void *top, void *fromp);

extern void
SFRotationNativeSet(void *p, struct Uni_String *sv);

extern void *
SFVec3fNativeNew(void);

extern void
SFVec3fNativeDelete(void *p);

extern void
SFVec3fNativeAssign(void *top, void *fromp);

extern void
SFVec3fNativeSet(void *p, struct Uni_String *sv);

extern void *
SFVec2fNativeNew(void);

extern void
SFVec2fNativeDelete(void *p);

extern void
SFVec2fNativeAssign(void *top, void *fromp);

extern void
SFVec2fNativeSet(void *p, struct Uni_String *sv);

extern void *
SFImageNativeNew(void);

extern void
SFImageNativeDelete(void *p);

extern void
SFImageNativeAssign(void *top, void *fromp);

extern void
SFImageNativeSet(void *p, struct Uni_String *sv);

extern void *
SFColorNativeNew(void);

extern void
SFColorNativeDelete(void *p);

extern void
SFColorNativeAssign(void *top, void *fromp);

extern void
SFColorNativeSet(void *p, struct Uni_String *sv);

#endif /* __jsNative_h__ */
