/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#ifndef __SENSINTERPS_H__
#define __SENSINTERPS_H__


#include "headers.h"
#include "XSUB.h"

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

#include "LinearAlgebra.h"
#include "quaternion.h"
#include "sounds.h"


#define ASLEN 500


float
return_Duration(int indx);

void
do_active_inactive(int *act,
				   double *inittime,
				   double *startt,
				   double *stopt,
				   int loop,
				   float myDuration,
				   float speed);

int
find_key(int kin, float frac, float *keys);

void
do_OintScalar(void *node);

void
do_GeoOint(void *node);

void
do_OintCoord(void *node);

void
do_Oint3(void *node);

void
do_Oint4(void *node);

void
do_AudioTick(struct VRML_AudioClip *node);

void
do_TimeSensorTick(struct VRML_TimeSensor *node);

void
do_ProximitySensorTick(struct VRML_ProximitySensor *node);

void
do_MovieTextureTick(struct VRML_MovieTexture *node);

void
do_Anchor(struct VRML_Anchor *node,
		  char *ev,
		  int over);

void
do_TouchSensor(struct VRML_TouchSensor *px,
			   char *typ,
			   int over);

void
do_GeoTouchSensor(struct VRML_GeoTouchSensor *px, char *typ, int over);

void
do_PlaneSensor(struct VRML_PlaneSensor *px,
			   char *typ,
			   int over);

void
do_CylinderSensor(struct VRML_CylinderSensor *px,
				  char *typ,
				  int over);

void
do_SphereSensor(struct VRML_SphereSensor *px,
				char *typ,
				int over);



#endif /* __SENSINTERPS_H__ */
