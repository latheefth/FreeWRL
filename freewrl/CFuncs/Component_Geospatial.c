/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Geospatial Component

*********************************************************************/

/* nodes:

GeoCoordinate
	- looks like Coordinate
	- defaultContainerType  coord
	- virt_Coordinate contains all NULLs. 

GeoElevationGrid
	- looks like ElevationGrid
	- defaultContainerType	geometry
	- member of RendC
	- member of GenPolyRepC
	- member of CollisionC
	- member of RendRayC
	- should mimic struct X3D_Virt virt_ElevationGrid = { NULL,(void *)render_ElevationGrid,NULL,NULL,(void *)rendray_ElevationGrid,(void *)make_ElevationGrid,NULL,NULL,(void *)collide_ElevationGrid,NULL};

GeoLocation
NOT MAPPED

GeoLOD
	- looks like LOD
	- defaultContainerType children
	- member of ChildC
	- should mimic struct X3D_Virt virt_ElevationGrid = { NULL,(void *)render_ElevationGrid,NULL,NULL,(void *)rendray_ElevationGrid,(void *)make_ElevationGrid,NULL,NULL,(void *)collide_ElevationGrid,NULL};



GeoMetadata
NOT MAPPED

GeoOrigin
NOT MAPPED

GeoPositionInterpolator
	- looks like positionInterpolator
	- defaultContainerType children
	- should mimic struct X3D_Virt virt_PositionInterpolator = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	

GeoTouchSensor
	- looks like touchSensor
	- defaultContainerType children
	- should mimic struct X3D_Virt virt_TouchSensor = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	

GeoViewpoint
	- looks like viewpoint
	- defaultContainerType children
	- member of PrepC
	- should mimic struct X3D_Virt virt_Viewpoint = { (void *)prep_Viewpoint,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};



*/

#include "headers.h"
#include "Bindable.h"
#include "Collision.h"
#include "Component_Geospatial.h"
#include "OpenGL_Utils.h"

int GeoVerbose = 0;

void make_GeoElevationGrid (struct X3D_GeoElevationGrid * node) {
	printf ("make_GeoElevationGrid\n");
}

void render_GeoElevationGrid (struct X3D_GeoElevationGrid * node) {
	printf ("render_GeoElevationGrid\n");
}

void fin_GeoLocation (struct X3D_GeoLocation *node) {
	printf ("fin_GeoLocation\n");
}

void child_GeoLOD (struct X3D_GeoLOD *node) {
	printf ("child_GeoLOD\n");
}

void child_GeoLocation (struct X3D_GeoLocation *node) {
	printf ("child_GeoLocation\n");
}

void changed_GeoLocation ( struct X3D_GeoLocation *node) { 
	printf ("changed_GeoLocation\n");
}


void collide_GeoElevationGrid (struct X3D_GeoElevationGrid *node) {
	printf ("collide_GeoElevationGrid\n");
}

/* GeoPositionInterpolator                                              */
/* Called during the "events_processed" section of the event loop,      */
/* so this is called ONLY when there is something required to do, thus  */
/* there is no need to look at whether it is active or not              */

void do_GeoPositionInterpolator (void *node) {
        struct X3D_GeoPositionInterpolator *px;
        
        /* remember to POSSIBLE_PROTO_EXPANSION(node->geoOrigin, tmpN) */
	printf ("do_GeoPositionInterpolator\n");
}
        

/* void do_GeoTouchSensor (struct X3D_GeoTouchSensor *node, int ev, int over) {*/
void do_GeoTouchSensor ( void *ptr, int ev, int but1, int over) {

struct X3D_GeoTouchSensor *node = (struct X3D_GeoTouchSensor *)ptr;

        /* remember to POSSIBLE_PROTO_EXPANSION(node->geoOrigin, tmpN) */
	printf ("do_GeoTouchSensor\n");
        
}; 

void
bind_geoviewpoint (struct X3D_GeoViewpoint *vp) {
	printf ("bind_geoviewpoint\n");
}
