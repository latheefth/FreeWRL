#ifndef __VIEWER_H_
#define __VIEWER_H_

/*
 * $Id$
 *
 */


#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <ctype.h>
#include <string.h>
#include <math.h>

#ifdef AQUA 
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "Structs.h"
#include "headers.h"
#include "LinearAlgebra.h"
#include "quaternion.h"

#define NONE 0
#define EXAMINE 1
#define WALK 2
#define EXFLY 3
#define FLY 4

#define PRESS "PRESS"
#define PRESS_LEN 5

#define DRAG "DRAG"
#define DRAG_LEN 4

#define RELEASE "RELEASE"
#define RELEASE_LEN 7


/* extern struct pt ViewerPosition; */
/* extern struct orient ViewerOrientation; */


typedef struct viewer_walk {
	double SX;
	double SY;
	double XD;
	double YD;
	double ZD;
	double RD;
} VRML_Viewer_Walk;


typedef struct viewer_examine {
	struct pt Origin;
	Quaternion OQuat;
	Quaternion SQuat;
	double ODist;
	double SY;
} VRML_Viewer_Examine;


/* Modeled after Descent(tm) ;) */
typedef struct viewer_fly {
	struct pt Velocity;
	struct pt AVelocity;
	double lasttime;
} VRML_Viewer_Fly;


typedef struct viewer {
	struct pt Pos;
	struct pt AntiPos;
	Quaternion Quat;
	Quaternion AntiQuat;
	/* struct VRML_NavigationInfo navi; */
	int headlight;
	double speed;
	double Dist;
	double eyehalf;
	double eyehalfangle;
	unsigned int buffer;
	VRML_Viewer_Walk *walk;
	VRML_Viewer_Examine *examine;
	VRML_Viewer_Fly *fly;
} VRML_Viewer;


void
viewer_init(
			VRML_Viewer *viewer,
			int type
			);

unsigned int
get_buffer(VRML_Viewer *viewer);

void
set_buffer(
		   VRML_Viewer *viewer,
		   const unsigned int buffer
		   );

int
get_headlight(VRML_Viewer *viewer);

void
toggle_headlight(VRML_Viewer *viewer);

int
use_keys(void);

void
set_eyehalf(
			VRML_Viewer *viewer,
			const double eyehalf,
			const double eyehalfangle
			);

void
set_viewer_type(const int type);

void
resolve_pos(VRML_Viewer *viewer);

void
xy2qua(
	   Quaternion *ret,
	   VRML_Viewer *viewer,
	   const double x,
	   const double y
	   );

void
viewer_togl(
			VRML_Viewer *viewer,
			double fieldofview
			);

void
set_stereo_offset(
				  unsigned int buffer,
				  const double eyehalf,
				  const double eyehalfangle,
				  double fieldofview
				  );





#endif /* __VIEWER_H_ */
