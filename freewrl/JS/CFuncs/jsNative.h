#ifndef __jsNative_h__
#define __jsNative_h__

#include <EXTERN.h>
#include <perl.h>

#include "Structs.h" /* FreeWRL C structs */

/*
 * for now, could try setting magic to pid since we only need one browser
 * per process -- but is it really needed ???
 */
typedef struct _BrowserNative {
	int magic; /* does this really do anything ??? */
	SV *sv_js;
} BrowserNative;

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


extern void
doPerlCallMethod(SV *sv, const char *methodName);

extern void
doPerlCallMethodVA(SV *sv, const char *methodName, const char *format, ...);


/*
 * Adds additional (touchable) property to instance of a native
 * type.
 */
extern JSBool
addECMANativeProperty(void *cx,
					  void *glob,
					  char *name);

extern JSBool
addAssignProperty(void *cx,
				  void *glob,
				  char *name,
				  char *str);


extern void *
SFRotationNativeNew(void);

extern void
SFRotationNativeDelete(void *p);

extern void
SFRotationNativeAssign(void *top, void *fromp);

extern void
SFRotationNativeSet(void *p, SV *sv);

extern void *
SFVec3fNativeNew(void);

extern void
SFVec3fNativeDelete(void *p);

extern void
SFVec3fNativeAssign(void *top, void *fromp);

extern void
SFVec3fNativeSet(void *p, SV *sv);

extern void *
SFVec2fNativeNew(void);

extern void
SFVec2fNativeDelete(void *p);

extern void
SFVec2fNativeAssign(void *top, void *fromp);

extern void
SFVec2fNativeSet(void *p, SV *sv);

extern void *
SFImageNativeNew(void);

extern void
SFImageNativeDelete(void *p);

extern void
SFImageNativeAssign(void *top, void *fromp);

extern void
SFImageNativeSet(void *p, SV *sv);

extern void *
SFColorNativeNew(void);

extern void
SFColorNativeDelete(void *p);

extern void
SFColorNativeAssign(void *top, void *fromp);

extern void
SFColorNativeSet(void *p, SV *sv);

#endif /* __jsNative_h__ */
