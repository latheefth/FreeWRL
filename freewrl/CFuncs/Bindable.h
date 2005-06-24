/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

Bindable nodes - Background, Fog, NavigationInfo, Viewpoint.

******************************************/


#include "EXTERN.h"
#include "perl.h"
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

#include "headers.h"
#include "Textures.h"

/* Bind stack */
#define MAX_STACK 20


/* Bindables, Viewpoint, NavigationInfo, Background and Fog */
#define BACKGROUND 1
#define VIEWPOINT  2
#define NAVIGATIONINFO 3
#define FOG        4
#define GEOVIEWPOINT 5
extern unsigned long int *fognodes;
extern unsigned long int *backgroundnodes;
extern unsigned long int *navnodes;
extern unsigned long int *viewpointnodes;
extern int totfognodes, totbacknodes, totnavnodes, totviewpointnodes;
extern int currboundvpno;

extern int viewpoint_tos;
extern unsigned int viewpoint_stack[];

void
reset_upvector(void);

void
set_naviinfo(struct VRML_NavigationInfo *node);

void
send_bind_to(int nodetype, void *node, int value);

void
bind_node(void *node, unsigned int setBindofst, int isboundofst, int *tos, unsigned int *stack);

void
render_Fog(struct VRML_Fog *node);

void
render_NavigationInfo(struct VRML_NavigationInfo *node);

void
render_Viewpoint(struct VRML_Viewpoint *node);

void
render_GeoViewpoint(struct VRML_GeoViewpoint *node);

void
render_Background(struct VRML_Background *node);


