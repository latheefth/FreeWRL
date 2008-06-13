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
	- looks like a Transform.

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

static int GeoVerbose = 0;

static int geoInitialized = FALSE;

#define INITIALIZE_GEOSPATIAL \
	if (!geoInitialized) { \
		printf ("initializing GeoSpatial code\n"); \
		geoInitialized = TRUE; \
	}

/* calculate a translation that moves a Geo node to local space */
static void GeoMove(void *geoOrigin, struct Multi_Int32* geoSystem, struct SFVec3d geoCoords, struct SFVec3d *outCoords) {
	struct SFVec3d *gcc;

	outCoords->c[0] = (double) 0.0; outCoords->c[1] = (double) 0.0; outCoords->c[2] = (double) 0.0;

	/* check the GeoOrigin attached node */
	if (geoOrigin == NULL) {
		ConsoleMessage ("GeoMove, null origin for Geospatial node");
		return;
	}

	if (X3D_GEOORIGIN(geoOrigin)->_nodeType != NODE_GeoOrigin) {
		ConsoleMessage ("GeoMove, expected a GeoOrigin, found a %s",stringNodeType(X3D_GEOORIGIN(geoOrigin)->_nodeType));
		return;
	}

	printf ("GeoMove, past step 1\n");
	gcc = &X3D_GEOORIGIN(geoOrigin)->geoCoords;
	printf ("GeoMove, origin coords %lf %lf %lf, this coords %lf %lf %lf\n",gcc->c[0],gcc->c[1],gcc->c[2],
			geoCoords.c[0],geoCoords.c[1],geoCoords.c[2]);
		

}

/* compileGeosystem - encode the return value such that srf->p[x] is...
			0:	spatial reference frame	(GEOSP_UTM, GEOSP_GC, GEOSP_GD);
			1:	spatial coordinates (defaults to GEOSP_WE)
			2:	UTM zone number, 1..60. ID_UNDEFINED = not specified
			3:	"S" - value is TRUE, not S, value is FALSE */

static void compile_geoSystem (int nodeType, struct Multi_String *args, struct Multi_Int32 *srf) {
	int i;
	indexT this_srf = ID_UNDEFINED;
	indexT this_srf_ind = ID_UNDEFINED;

	/* malloc the area required for internal settings, if required */
	if (srf->p==NULL) {
		srf->n=4;
		srf->p=MALLOC(sizeof(int) * 4);
	}

	/* set these as defaults */
	srf->p[0] = GEOSP_GD; 
	srf->p[1] = GEOSP_WE;
	srf->p[2] = ID_UNDEFINED;
	srf->p[3] = ID_UNDEFINED;

	/* if nothing specified, we just use these defaults */
	if (args->n==0) return;

	/* first go through, and find the Spatial Reference Frame, GD, UTM, or GC */
	for (i=0; i<args->n; i++) {
		/* printf ("geoSystem args %d %s\n",i, args->p[i]->strptr); */
		indexT tc = findFieldInGEOSPATIAL(args->p[i]->strptr);

		if ((tc == GEOSP_GD) || (tc == GEOSP_GDC)) {
			this_srf = GEOSP_GD;
			this_srf_ind = i;
		} else if ((tc == GEOSP_GC) || (tc == GEOSP_GCC)) {
			this_srf = GEOSP_GC;
			this_srf_ind = i;
		} else if (tc == GEOSP_UTM) {
			this_srf = GEOSP_UTM;
			this_srf_ind = i;
		}
	}

	/* did we find a GC, GD, or UTM? */
	if (this_srf == ID_UNDEFINED) {
		ConsoleMessage ("geoSystem in node %s,  must have GC, GD or UTM",stringNodeType(nodeType));
		return;
	}

	srf->p[0] = this_srf;
	/* go through and ensure that we have the correct parameters for this spatial reference frame */
	if (this_srf == GEOSP_GC) {
		if (args->n != 1) {
			ConsoleMessage ("geoSystem in node %s, GC not allowed geospatial coordinates",stringNodeType(nodeType));
		}
		srf->p[1] = ID_UNDEFINED;
		
	} else if (this_srf == GEOSP_GD) {
		indexT this_p_ind;

		/* is there an optional argument? */
		if (this_srf_ind == 0) this_p_ind = 1; else this_p_ind = 0;
		if (args->n == 2) {
			indexT tc = findFieldInGEOSPATIAL(args->p[this_p_ind]->strptr);
			switch (tc) {
				case ID_UNDEFINED:
				case GEOSP_GC:
				case GEOSP_GCC:
				case GEOSP_GD:
				case GEOSP_GDC:
				case GEOSP_UTM:
					ConsoleMessage("expected valid GC parameter in node %s",stringNodeType(nodeType));
					srf->p[1] = GEOSP_WE;
					break;

				default:
					srf->p[1] = tc;
			}
		} else {
			if (args->n != 1) 
				ConsoleMessage ("geoSystem GC can take only 1 extra parameter in node %s",stringNodeType(nodeType));
			srf->p[1] = GEOSP_WE;
		}
	} else {
		/* this must be UTM */
		/* encode the return value such that srf->p[x] is...
			0:	spatial reference frame	(GEOSP_UTM, GEOSP_GC, GEOSP_GD);
			1:	spatial coordinates (defaults to GEOSP_WE)
			2:	UTM zone number, 1..60. ID_UNDEFINED = not specified
			3:	"S" - value is TRUE, not S, value is FALSE */
		/* first go through, and find the Spatial Reference Frame, GD, UTM, or GC */
		for (i=0; i<args->n; i++) {
			if (i != this_srf_ind) {
				if (strcmp ("S",args->p[i]->strptr) == 0) {
					srf->p[3] = TRUE;
				} else if (args->p[i]->strptr[0] = 'Z') {
					int zone = -1;
					sscanf(args->p[i]->strptr,"Z%d",&zone);
					/* printf ("zone found as %d\n",zone); */
					srf->p[2] = zone;
				} else { 
					indexT tc = findFieldInGEOSPATIAL(args->p[i]->strptr);
					switch (tc) {
						case ID_UNDEFINED:
						case GEOSP_GC:
						case GEOSP_GCC:
						case GEOSP_GD:
						case GEOSP_GDC:
						case GEOSP_UTM:
							ConsoleMessage("expected valid UTM ellipsoid parameter in node %s",stringNodeType(nodeType));
							srf->p[1] = GEOSP_WE;
						break;

					default:
						srf->p[1] = tc;
					}
				}
			}
					
		}		
	}

}

/***********************************************************************/

void compile_GeoCoordinate (struct X3D_GeoCoordinate * node) {
	printf ("compiling GeoCoordinate\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoElevationGrid (struct X3D_GeoElevationGrid * node) {
	printf ("compiling GeoElevationGrid\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoLocation (struct X3D_GeoLocation * node) {
        struct SFVec3d translatedCoords;
	printf ("compiling GeoLocation\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	GeoMove(node->geoOrigin, &node->__geoSystem, node->geoCoords, &translatedCoords);

	MARK_NODE_COMPILED
}

void compile_GeoLOD (struct X3D_GeoLOD * node) {
	printf ("compiling GeoLOD\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoMetadata (struct X3D_GeoMetadata * node) {
	printf ("compiling GeoMetadata\n");

	MARK_NODE_COMPILED
}

void compile_GeoOrigin (struct X3D_GeoOrigin * node) {
	printf ("compiling GeoOrigin\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoPositionInterpolator (struct X3D_GeoPositionInterpolator * node) {
	printf ("compiling GeoPositionInterpolator\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoTouchSensor (struct X3D_GeoTouchSensor * node) {
	printf ("compiling GeoTouchSensor\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}


void compile_GeoViewpoint (struct X3D_GeoViewpoint * node) {
	printf ("compiling GeoViewpoint\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

/**************************************************************************/
void make_GeoElevationGrid (struct X3D_GeoElevationGrid * node) {
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}

void collide_GeoElevationGrid (struct X3D_GeoElevationGrid *node) {
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}

void render_GeoElevationGrid (struct X3D_GeoElevationGrid * node) {
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}

/**************************************************************************/
void child_GeoLocation (struct X3D_GeoLocation *node) {
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}

void changed_GeoLocation ( struct X3D_GeoLocation *node) { 
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}

void fin_GeoLocation (struct X3D_GeoLocation *node) {
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}

/**************************************************************************/
void child_GeoLOD (struct X3D_GeoLOD *node) {
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}

/* GeoPositionInterpolator                                              */
/* Called during the "events_processed" section of the event loop,      */
/* so this is called ONLY when there is something required to do, thus  */
/* there is no need to look at whether it is active or not              */

void do_GeoPositionInterpolator (void *this) {
        struct X3D_GeoPositionInterpolator *node = this;
        
        /* remember to POSSIBLE_PROTO_EXPANSION(node->geoOrigin, tmpN) */
	printf ("do_GeoPositionInterpolator\n");

	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}
        

/* void do_GeoTouchSensor (struct X3D_GeoTouchSensor *node, int ev, int over) {*/
void do_GeoTouchSensor ( void *ptr, int ev, int but1, int over) {

struct X3D_GeoTouchSensor *node = (struct X3D_GeoTouchSensor *)ptr;


	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
        /* remember to POSSIBLE_PROTO_EXPANSION(node->geoOrigin, tmpN) */
	printf ("do_GeoTouchSensor\n");
        
}; 

void
bind_geoviewpoint (struct X3D_GeoViewpoint *node) {
	printf ("bind_geoviewpoint\n");

	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
}
