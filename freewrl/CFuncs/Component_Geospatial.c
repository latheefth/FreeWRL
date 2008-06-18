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
	

*/

#include "headers.h"
#include "Bindable.h"
#include "Collision.h"
#include "Component_Geospatial.h"
#include "OpenGL_Utils.h"


/* defines used to get a SFVec3d into/outof a function that expects a MFVec3d */
#define MF_SF_TEMPS	struct Multi_Vec3d *min, *mout;
#define INIT_MF_FROM_SF(myField) \
	min->n = 1; \
	min->p = MALLOC(sizeof (struct SFVec3d)); \
	min->p[0].c[0] = node-> myField .c[0];\
	min->p[0].c[1] = node-> myField .c[1];\
	min->p[0].c[2] = node-> myField .c[2];\
	mout->n=0; mout->p = NULL;
#define MF_FIELD_IN_OUT min, mout
#define COPY_MF_TO_SF(myField) \
	printf ("COPY_MF_TO_SF, mout->n %d\n",mout->n); \
	node-> myField .c[0] = mout->p[0].c[0]; \
	node-> myField .c[1] = mout->p[0].c[1]; \
	node-> myField .c[2] = mout->p[0].c[2]; \
	FREE_IF_NZ(min->p); FREE_IF_NZ(mout->p);


#define RADIANS_PER_DEGREE (double)0.0174532925199432957692
#define DEGREES_PER_RADIAN (double)57.2957795130823208768

/* for ellipsoid conversions */
#define GEOSP_AA_A	(double)6377563.396
#define GEOSP_AA_F	(double)299.3249646
#define GEOSP_AM_A	(double)6377340.189
#define GEOSP_AM_F	(double)299.3249646
#define GEOSP_AN_A	(double)6378160
#define GEOSP_AN_F	(double)298.25
#define GEOSP_BN_A	(double)6377483.865
#define GEOSP_BN_F	(double)299.1528128
#define GEOSP_BR_A	(double)6377397.155
#define GEOSP_BR_F	(double)299.1528128
#define GEOSP_CC_A	(double)6378206.4
#define GEOSP_CC_F	(double)294.9786982
#define GEOSP_CD_A	(double)6378249.145
#define GEOSP_CD_F	(double)293.465
#define GEOSP_EA_A	(double)6377276.345
#define GEOSP_EA_F	(double)300.8017
#define GEOSP_EB_A	(double)6377298.556
#define GEOSP_EB_F	(double)300.8017
#define GEOSP_EC_A	(double)6377301.243
#define GEOSP_EC_F	(double)300.8017
#define GEOSP_ED_A	(double)6377295.664
#define GEOSP_ED_F	(double)300.8017
#define GEOSP_EE_A	(double)6377304.063
#define GEOSP_EE_F	(double)300.8017
#define GEOSP_EF_A	(double)6377309.613
#define GEOSP_EF_F	(double)300.8017
#define GEOSP_FA_A	(double)6378155
#define GEOSP_FA_F	(double)298.3
#define GEOSP_HE_A	(double)6378200
#define GEOSP_HE_F	(double)298.3
#define GEOSP_HO_A	(double)6378270
#define GEOSP_HO_F	(double)297
#define GEOSP_ID_A	(double)6378160
#define GEOSP_ID_F	(double)298.247
#define GEOSP_IN_A	(double)6378388
#define GEOSP_IN_F	(double)297
#define GEOSP_KA_A	(double)6378245
#define GEOSP_KA_F	(double)298.3
#define GEOSP_RF_A	(double)6378137
#define GEOSP_RF_F	(double)298.257222101
#define GEOSP_SA_A	(double)6378160
#define GEOSP_SA_F	(double)298.25
#define GEOSP_WD_A	(double)6378135
#define GEOSP_WD_F	(double)298.26
#define GEOSP_WE_A	(double)6378137
#define GEOSP_WE_F	(double)298.257223563

#define ELLIPSOID(typ) \
	case typ: ellipsoid(inCoords,outCoords,typ##_A, typ##_F); break;

#define UTM_ELLIPSOID(typ) \
	case typ: Utm_Gd (inCoords, tmpCoords, typ##_A, typ##_F, geoSystem->p[3], geoSystem->p[2]); \
		  ellipsoid(tmpCoords,outCoords,typ##_A, typ##_F); break;

static int GeoVerbose = 0;
static struct SFVec3d geoViewPointCenter = {(double)0.0, (double)0.0, (double)0.0};

static struct X3D_GeoOrigin *geoorigin = NULL;

static void compile_geoSystem (int nodeType, struct Multi_String *args, struct Multi_Int32 *srf);
static void moveCoords(struct Multi_Int32*, struct Multi_Vec3d *, struct Multi_Vec3d *);
static void ellipsoid (struct Multi_Vec3d *, struct Multi_Vec3d *, double, double);


/* convert GD ellipsiod to GC coordinates */
static void ellipsoid (struct Multi_Vec3d *inc, struct Multi_Vec3d *outc, double sma, double flatten) {
	int i;
	double A = sma;
	double A2 = sma*sma;
	double F = (double)(1/flatten);
	double C = A*((double)1.0 - F);
	double C2 = C*C;
	double Eps2 = F*((double)2.0 - F);
	double Eps25 = (double) 0.25 * Eps2;

	int latitude = 0;
	int longitude = 1;
	int elevation = 2;

	double source_lat;
	double source_lon;
	double slat;
	double slat2;
	double clat;
	double Rn;
	double RnPh;

	/* enough room for output? */
	if (outc->n < inc->n) {
		FREE_IF_NZ(outc->p);
		outc->p = MALLOC(sizeof (struct SFVec3d) * inc->n);
		outc->n = inc->n;
	}

printf ("ellipsoid, have n of %d\n",inc->n);
	for (i=0; i<inc->n; i++) {
		printf ("ellipsoid, ining %lf %lf %lf\n",inc->p[i].c[0], inc->p[i].c[1], inc->p[i].c[2]);
		source_lat = RADIANS_PER_DEGREE * inc->p[i].c[latitude];
		source_lon = RADIANS_PER_DEGREE * inc->p[i].c[longitude];
	
		slat = sin(source_lat);
		slat2 = slat*slat;
		clat = cos(source_lat);
	
		/* square root approximation for Rn */
		Rn = A / ( (.25 - Eps25 * slat2 + .9999944354799/4) + (.25-Eps25 * slat2)/(.25 - Eps25 * slat2 + .9999944354799/4));
	
		RnPh = inc->p[i].c[elevation];
	
		outc->p[i].c[0] = RnPh * clat * cos(source_lon);
		outc->p[i].c[1] = RnPh * clat * sin(source_lon);
		outc->p[i].c[2] = ((C2 / A2) * Rn + RnPh) * slat;
		printf ("ellipsoid, outing %lf %lf %lf\n",outc->p[i].c[0], outc->p[i].c[1], outc->p[i].c[2]);
	}
}


/* convert UTM to GC coordinates by converting to GD as an intermediary step */
static void Utm_Gd (struct Multi_Vec3d *inc, struct Multi_Vec3d *outc, double sma, double flatten, int hemisphere_north, int zone) {
        double source_x, source_y,u,su,cu,su2,xlon0,temp,phi1,sp,sp2,cp,cp2,tp,tp2,eta2,top,rn,b3,b4,b5,b6,d1,d2;
	int i;
	int latitude = 0;
	int longitude = 1;
	int elevation = 2;

	/* create the ERM constants. */
    	double A = sma;
	double F = 1.0/flatten;
	double C      = (A) * (1-F);
	double Eps2   = (F) * (2.0-F);
	double Eps25  = .25 * (Eps2);
	double EF     = F/(2.0-F);
	double Con    = (1.0-Eps2);
	double Con2   = 2 / (1.0-Eps2);
	double Con6   = .166666666666667;
	double Con24  = 4 * .0416666666666667/(1-Eps2);
	double Con120 = .00833333333333333;
	double Con720 = 4 * .00138888888888888/(1-Eps2);
	double polx1a = 1.0 - Eps2 / 4.0 - 3.0/64.0 * pow(Eps2,2) - 5.0/256.0 * pow(Eps2,3) - 175.0/16384.0 * pow(Eps2,4);
	double conap  = A * polx1a;
	double polx2a = 3.0/2.0 * EF - 27.0/32.0 * pow(EF,3);
	double polx4a = 21.0/16.0 * pow(EF,2) - 55.0/32.0 * pow(EF,4);
	double polx6a = 151.0/96.0 * pow(EF,3);
	double polx8a = 1097.0/512.0 * pow(EF,4);
	double polx2b = polx2a * 2.0 + polx4a * 4.0 + polx6a * 6.0 + polx8a * 8.0;
	double polx3b = polx4a * -8.0 - polx6a * 32.0- 80.0 *polx8a;
	double polx4b = polx6a * 32.0 + 192.0*polx8a;
	double polx5b = -128.0 *polx8a;
	double Epsp2 = 1.0;

printf ("NOTE: Epsp2 is declared as 1.0 - what should it be??\n");



	/* enough room for output? */
	if (outc->n < inc->n) {
		FREE_IF_NZ(outc->p);
		outc->p = MALLOC(sizeof (struct SFVec3d) * inc->n);
		outc->n = inc->n;
	}


        for(i=0;i<inc->n;i++) {

            outc->p[i].c[elevation] = inc->p[i].c[2];

            source_x = inc->p[i].c[0];

            source_x = (source_x - 500000.0)/.9996;

            if (hemisphere_north)
                source_y = inc->p[i].c[1] / .9996;
            else
                source_y = (inc->p[i].c[1] - 1.0E7)/.9996;

            u = source_y / conap;

            /* TEST U TO SEE IF AT POLES */

            su = sin(u);
            cu = cos(u);
            su2 = su * su;

            /* THE SNYDER FORMULA FOR PHI1 IS OF THE FORM
             PHI1=U+POLY2A*SIN(2U)+POLY3A*SIN(4U)+POLY4ASIN(6U)+...
             BY USING MULTIPLE ANGLE TRIGONOMETRIC IDENTITIES AND APPROPRIATE FACTORING
            JUST THE SINE AND COSINE ARE REQUIRED
             NOW READY TO GET PHI1
             */

            xlon0= ( 6.0 * ((double) zone) - 183.0) / DEGREES_PER_RADIAN;

            temp = polx2b + su2 * (polx3b + su2 * (polx4b + su2 * polx5b));

            phi1 = u + su * cu * temp;

             /* COMPUTE VARIABLE COEFFICIENTS FOR FINAL RESULT
                COMPUTE THE VARIABLE COEFFICIENTS OF THE LAT AND LON
                EXPANSIONS */

            sp = sin(phi1);
            cp = cos(phi1);
            tp = sp / cp;
            tp2 = tp * tp;
            sp2 = sp * sp;
            cp2 = cp * cp;
            eta2 = Epsp2 * cp2;

            top = .25-(sp2*(Eps2 / 4));

             /* inline sq root*/

            rn = A / ( (.25 - Eps25 * sp2 + .9999944354799/4) +
                (.25-Eps25 * sp2)/(.25 - Eps25 * sp2 + .9999944354799/4));

            b3 = 1.0 + tp2 + tp2 + eta2;
            b4 = 5 + tp2 * (3 - 9 * eta2) + eta2 * ( 1 - 4 * eta2);

            b5 = 5 + tp2 * (tp2 * 24.0 + 28.0);

            b5 += eta2 * (tp2 * 8.0 + 6.0);

            b6 = 46.0 - 3.0 * eta2 + tp2 * (-252.0 - tp2 * 90.0);

            b6 = eta2 * (b6 + eta2 * tp2 * (tp2 * 225.0 - 66.0));
            b6 += 61.0 + tp2 * (tp2 * 45.0 + 90.0);

            d1 = source_x / rn;
            d2 = d1 * d1;

            outc->p[i].c[latitude] = phi1 - tp * top * (d2 * (Con2 + d2 * ((-Con24) * b4 + d2 *
                        Con720 * b6)));

            outc->p[i].c[longitude] = xlon0 + d1 * (1.0 + d2 * (-Con6 * b3 + d2 * Con120 * b5)) / cp;

             /* TRANSVERSE MERCATOR COMPUTATIONS DONE */

            outc->p[i].c[latitude] *= DEGREES_PER_RADIAN;

            outc->p[i].c[longitude] *= DEGREES_PER_RADIAN;

        } // end for
}

/* take a set of coords, and a geoSystem, and create a set of moved coords */
static void moveCoords (struct Multi_Int32* geoSystem, struct Multi_Vec3d *inCoords, struct Multi_Vec3d *outCoords) {
	struct Multi_Vec3d *tmpCoords; /* used UTM conversions */

	int i;
#define ENSURE_SPACE(variableInQuestion) \
	/* enough room for output? */ \
printf ("ENSURE_SPACE %d and %d\n", variableInQuestion ->n, inCoords->n); \
	if (variableInQuestion ->n < inCoords->n) { \
		if (variableInQuestion ->p != NULL) { \
			FREE_IF_NZ(variableInQuestion->p); \
		} \
		variableInQuestion ->p = MALLOC(sizeof (struct SFVec3d) * inCoords->n); \
		variableInQuestion ->n = inCoords->n; \
	} 

	/* tmpCoords used for UTM coding */
	tmpCoords->n=0; tmpCoords->p=NULL;

	/* make sure the output has enough space for our converted data */
	ENSURE_SPACE(outCoords)

	switch (geoSystem->p[0]) {
		case  GEOSP_GD:

				/* GD_ellipsoid_convert (inCoords, outCoords); */
				switch (geoSystem->p[1]) {
					ELLIPSOID(GEOSP_AA)
					ELLIPSOID(GEOSP_AM)
					ELLIPSOID(GEOSP_AN)
					ELLIPSOID(GEOSP_BN)
					ELLIPSOID(GEOSP_BR)
					ELLIPSOID(GEOSP_CC)
					ELLIPSOID(GEOSP_CD)
					ELLIPSOID(GEOSP_EA)
					ELLIPSOID(GEOSP_EB)
					ELLIPSOID(GEOSP_EC)
					ELLIPSOID(GEOSP_ED)
					ELLIPSOID(GEOSP_EE)
					ELLIPSOID(GEOSP_EF)
					ELLIPSOID(GEOSP_FA)
					ELLIPSOID(GEOSP_HE)
					ELLIPSOID(GEOSP_HO)
					ELLIPSOID(GEOSP_ID)
					ELLIPSOID(GEOSP_IN)
					ELLIPSOID(GEOSP_KA)
					ELLIPSOID(GEOSP_RF)
					ELLIPSOID(GEOSP_SA)
					ELLIPSOID(GEOSP_WD)
					ELLIPSOID(GEOSP_WE)
					default: printf ("unknown ellipsoid: %s\n", stringGEOSPATIALType(geoSystem->p[1]));

				}
			break;
		case GEOSP_GC:
			/* an earth-fixed geocentric coord; no conversion required */
			for (i=0; i< inCoords->n; i++) {
				outCoords->p[i].c[0] = inCoords->p[i].c[0];
				outCoords->p[i].c[1] = inCoords->p[i].c[1];
				outCoords->p[i].c[2] = inCoords->p[i].c[2];
			}

			break;
		case GEOSP_UTM:
				ENSURE_SPACE(tmpCoords)

				/* first, convert UTM to GC, then GD, then GD to GC */
				/* see the compileGeosystem function for geoSystem fields */
				switch (geoSystem->p[1]) {
					UTM_ELLIPSOID(GEOSP_AA)
					UTM_ELLIPSOID(GEOSP_AM)
					UTM_ELLIPSOID(GEOSP_AN)
					UTM_ELLIPSOID(GEOSP_BN)
					UTM_ELLIPSOID(GEOSP_BR)
					UTM_ELLIPSOID(GEOSP_CC)
					UTM_ELLIPSOID(GEOSP_CD)
					UTM_ELLIPSOID(GEOSP_EA)
					UTM_ELLIPSOID(GEOSP_EB)
					UTM_ELLIPSOID(GEOSP_EC)
					UTM_ELLIPSOID(GEOSP_ED)
					UTM_ELLIPSOID(GEOSP_EE)
					UTM_ELLIPSOID(GEOSP_EF)
					UTM_ELLIPSOID(GEOSP_FA)
					UTM_ELLIPSOID(GEOSP_HE)
					UTM_ELLIPSOID(GEOSP_HO)
					UTM_ELLIPSOID(GEOSP_ID)
					UTM_ELLIPSOID(GEOSP_IN)
					UTM_ELLIPSOID(GEOSP_KA)
					UTM_ELLIPSOID(GEOSP_RF)
					UTM_ELLIPSOID(GEOSP_SA)
					UTM_ELLIPSOID(GEOSP_WD)
					UTM_ELLIPSOID(GEOSP_WE)
					default: printf ("unknown ellipsoid: %s\n", stringGEOSPATIALType(geoSystem->p[1]));
				}
				FREE_IF_NZ(tmpCoords->p)
			break;
		default :
			printf ("incorrect geoSystem field, %s\n",stringGEOSPATIALType(geoSystem->p[0]));
			return;

	}
}
#define INITIALIZE_GEOSPATIAL \
	if (geoorigin == NULL) { \
		initializeGeospatial((struct X3D_GeoOrigin **) &node->geoOrigin); \
	} else if (X3D_GEOORIGIN(node->geoOrigin) != geoorigin) { \
		if (node->geoOrigin != NULL) ConsoleMessage ("have more than 1 GeoOrigin..."); \
		node->geoOrigin = geoorigin; /* make all same */ \
	}

static void initializeGeospatial (struct X3D_GeoOrigin **nodeptr)  {
	MF_SF_TEMPS
	

	printf ("initializing GeoSpatial code\n"); 
	if (*nodeptr != NULL) {
		if (X3D_GEOORIGIN(*nodeptr)->_nodeType != NODE_GeoOrigin) {
			printf ("expected a GeoOrigin node, but got a node of type %s\n",
				X3D_GEOORIGIN(*nodeptr)->_nodeType);
			geoorigin = createNewX3DNode(NODE_GeoOrigin); /* dummy node, because of error */
		} else {
			geoorigin = X3D_GEOORIGIN(*nodeptr);
		}
	} else {
		/* printf ("expected a non-null geoOrigin, faking it\n"); */
		geoorigin = createNewX3DNode(NODE_GeoOrigin); /* dummy node, because none specified */
		*nodeptr = geoorigin;
	}

	compile_geoSystem (geoorigin->_nodeType, &geoorigin->geoSystem, &geoorigin->__geoSystem);
	#define node geoorigin
	INIT_MF_FROM_SF(__movedCoords)

printf ("calling moveCoords in initializeGeospatial\n");
	moveCoords(&geoorigin->__geoSystem, MF_FIELD_IN_OUT);
	COPY_MF_TO_SF(__movedCoords)
printf ("called moveCoords in initializeGeospatial\n");

printf ("geoOrigin, original coords %lf %lf %lf, moved %lf %lf %lf\n",
		geoorigin->geoCoords.c[0],
		geoorigin->geoCoords.c[1],
		geoorigin->geoCoords.c[2],
		geoorigin->__movedCoords.c[0],
		geoorigin->__movedCoords.c[1],
		geoorigin->__movedCoords.c[2]);
		

	printf ("initializeGeospatial, geoCoords %lf %lf %lf, ryup %d, geoSystem %d %d %d %d\n",
		geoorigin->geoCoords.c[0],
		geoorigin->geoCoords.c[1],
		geoorigin->geoCoords.c[2],
		geoorigin->rotateYUp,
		geoorigin->__geoSystem.p[0],
		geoorigin->__geoSystem.p[1],
		geoorigin->__geoSystem.p[2],
		geoorigin->__geoSystem.p[3]);
}

/* calculate a translation that moves a Geo node to local space */
static void GeoMove(struct X3D_GeoOrigin *geoOrigin, struct Multi_Int32* geoSystem, struct Multi_Vec3d *inCoords, struct Multi_Vec3d *outCoords) {
	int i;
	struct X3D_GeoOrigin * myOrigin;

	/* enough room for output? */
	if (outCoords->n < inCoords->n) {
		if (outCoords->n!=0) {
			FREE_IF_NZ(outCoords->p);
		}
		outCoords->p = MALLOC(sizeof (struct SFVec3d) * inCoords->n);
		outCoords->n = inCoords->n;
	}

	for (i=0; i<outCoords->n; i++) {
		outCoords->p[i].c[0] = (double) 0.0; outCoords->p[i].c[1] = (double) 0.0; outCoords->p[i].c[2] = (double) 0.0;
	}

	for (i=0; i<outCoords->n; i++) {
		printf ("start of GeoMove, inCoords %d: %lf %lf %lf\n",i, inCoords->p[i].c[0], inCoords->p[i].c[1], inCoords->p[i].c[2]);
	}


	/* check the GeoOrigin attached node */
	if (geoOrigin != NULL) {
		if (X3D_GEOORIGIN(geoOrigin)->_nodeType != NODE_GeoOrigin) {
			ConsoleMessage ("GeoMove, expected a GeoOrigin, found a %s",stringNodeType(X3D_GEOORIGIN(geoOrigin)->_nodeType));
			return;
		}

		myOrigin = geoOrigin; /* local one */
	} else {
		myOrigin = geoorigin; /* global one */
	}
		

printf ("calling moveCoords in GeoMove\n");
	moveCoords(geoSystem, inCoords, outCoords);
printf ("called moveCoords in GeoMove\n");
	for (i=0; i<outCoords->n; i++) {
printf ("GeoMove, before subtracting origin %lf %lf %lf\n",
outCoords->p[i].c[0], outCoords->p[i].c[1], outCoords->p[i].c[2]);

	outCoords->p[i].c[0] -= myOrigin->geoCoords.c[0];
	outCoords->p[i].c[1] -= myOrigin->geoCoords.c[1];
	outCoords->p[i].c[2] -= myOrigin->geoCoords.c[2];
	}
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
	MF_SF_TEMPS

	printf ("compiling GeoLocation\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	INIT_MF_FROM_SF(geoCoords)

	GeoMove(X3D_GEOORIGIN(node->geoOrigin), &node->__geoSystem, MF_FIELD_IN_OUT);

	COPY_MF_TO_SF(__movedCoords)

printf ("compile_GeoLocation, orig coords %lf %lf %lf, moved %lf %lf %lf\n", node->geoCoords.c[0], node->geoCoords.c[1], node->geoCoords.c[2], node->__movedCoords.c[0], node->__movedCoords.c[1], node->__movedCoords.c[2]);

	MARK_NODE_COMPILED
	printf ("compiled GeoLocation\n\n");
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

/**************************************************************************/

void compile_GeoViewpoint (struct X3D_GeoViewpoint * node) {
	MF_SF_TEMPS

	printf ("compiling GeoViewpoint\n");
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);
printf ("GeoViewpoint, position at start of compile is %lf %lf %lf\n",node->position.c[0], node->position.c[1], node->position.c[2]);

	INIT_MF_FROM_SF(position)

	GeoMove(X3D_GEOORIGIN(node->geoOrigin), &node->__geoSystem, MF_FIELD_IN_OUT);
	COPY_MF_TO_SF(__movedPosition)

	geoViewPointCenter.c[0] = -node->__movedPosition.c[0];
	geoViewPointCenter.c[1] = -node->__movedPosition.c[1];
	geoViewPointCenter.c[2] = -node->__movedPosition.c[2];

	MARK_NODE_COMPILED
	printf ("compiled GeoViewpoint\n\n");
}


void prep_GeoViewpoint (struct X3D_GeoViewpoint *node) {
	double a1;

	if (!render_vp) return;

	INITIALIZE_GEOSPATIAL

	 /* printf ("RVP, node %d ib %d sb %d gepvp\n",node,node->isBound,node->set_bind);
	 printf ("VP stack %d tos %d\n",viewpoint_tos, viewpoint_stack[viewpoint_tos]);
	 */

	/* check the set_bind eventin to see if it is TRUE or FALSE */
	/* code to perform binding is now in set_viewpoint. */

	if(!node->isBound) return;
	COMPILE_IF_REQUIRED

	/*
	printf ("Component_Nav, found VP is %d, (%s)\n",node,node->description->strptr);
	*/

	/* perform GeoViewpoint translations */
/*	glRotated(-node->orientation.r[3]/PI*180.0,node->orientation.r[0],node->orientation.r[1],
		node->orientation.r[2]); */
/*
	glTranslated(-node->__movedPosition.c[0],-node->__movedPosition.c[1],-node->__movedPosition.c[2]);
	printf ("GeoViewpoint, going to %lf %lf %lf\n",-node->__movedPosition.c[0],-node->__movedPosition.c[1],-node->__movedPosition.c[2]);
*/

	/* now, lets work on the GeoViewpoint fieldOfView */
	glGetIntegerv(GL_VIEWPORT, viewPort);
	if(viewPort[2] > viewPort[3]) {
		a1=0;
		fieldofview = node->fieldOfView/3.1415926536*180;
	} else {
		a1 = node->fieldOfView;
		a1 = atan2(sin(a1),viewPort[2]/((float)viewPort[3]) * cos(a1));
		fieldofview = a1/3.1415926536*180;
	}
	/* printf ("render_Viewpoint, bound to %d, fieldOfView %f \n",node,node->fieldOfView); */
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
	int nc = (node->children).n;
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED

	OCCLUSIONTEST

	DIRECTIONAL_LIGHT_SAVE

	/* any children at all? */
	if (nc==0) return;

	/* {
		int x;
		struct X3D_Node *xx;

		printf ("child_GeoLocation, this %d \n",node);
		for (x=0; x<nc; x++) {
			xx = X3D_NODE(node->children.p[x]);
			printf ("	ch %d type %s dist %f\n",node->children.p[x],stringNodeType(xx->_nodeType),xx->_dist);
		}
	} */

	/* Check to see if we have to check for collisions for this transform. */

	/* should we go down here? */
	/* printf("transformChild %d render_blend %x renderFlags %x\n",
			node, render_blend, node->_renderFlags); */
	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((node->_renderFlags & VF_Proximity) != VF_Proximity) {
			return;
		}


	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	DIRLIGHTCHILDREN(node->children);

	/* now, just render the non-directionalLight children */

	/* printf ("GeoLocation %d, flags %d, render_sensitive %d\n",
			node,node->_renderFlags,render_sensitive); */

	#ifdef CHILDVERBOSE
		printf ("GeoLocation - doing normalChildren\n");
	#endif

	normalChildren(node->children);

	#ifdef CHILDVERBOSE
		printf ("GeoLocation - done normalChildren\n");
	#endif

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
		node->bboxCenter.c[0] = node->__movedCoords.c[0];
		node->bboxCenter.c[1] = node->__movedCoords.c[1];
		node->bboxCenter.c[2] = node->__movedCoords.c[2];

		/* pass the bounding box calculations on up the chain */
		propagateExtent(X3D_NODE(node));
		BOUNDINGBOX

	}


	DIRECTIONAL_LIGHT_OFF
}

void changed_GeoLocation ( struct X3D_GeoLocation *node) { 
	int i;
	int nc = ((node->children).n);
	struct X3D_Node *p;

	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED
	INITIALIZE_EXTENT

}

/* do transforms, calculate the distance */
void prep_GeoLocation (struct X3D_GeoLocation *node) {
	GLfloat my_rotation;
	GLfloat my_scaleO=0;
	int	recalculate_dist;

	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED

        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	 /* we recalculate distance on last pass, or close to it, and only
	 once per event-loop tick. we can do it on the last pass - the
	 render_sensitive pass, but when mouse is clicked (eg, moving in
	 examine mode, sensitive node code is not rendered. So, we choose
	 the second-last pass. ;-) */
	recalculate_dist = render_light;

	/* printf ("prep_GeoLocation, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

	/* do we have any geometry visible, and are we doing anything with geometry? */
	OCCLUSIONTEST

	if(!render_vp) {
                /* glPushMatrix();*/
		fwXformPush(node);

		/* TRANSLATION */

		glTranslated(node->__movedCoords.c[0]-geoViewPointCenter.c[0],
			node->__movedCoords.c[1]-geoViewPointCenter.c[1],
			node->__movedCoords.c[2]-geoViewPointCenter.c[2]);
		printf ("prep_GeoLoc trans to %lf %lf %lf\n",node->__movedCoords.c[0],node->__movedCoords.c[1],node->__movedCoords.c[2]);
		printf ("          (really to %lf %lf %lf)\n",node->__movedCoords.c[0]-geoViewPointCenter.c[0],
			node->__movedCoords.c[1]-geoViewPointCenter.c[1],
			node->__movedCoords.c[2]-geoViewPointCenter.c[2]);

		/* did either we or the Viewpoint move since last time? */
		if (recalculate_dist) {
			/* printf ("calling recordDistance for %d\n",node);*/
			recordDistance(node);
			/* printf ("ppv %d\n"g);*/

	       }
        }
}
void fin_GeoLocation (struct X3D_GeoLocation *node) {
	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED


	OCCLUSIONTEST

        if(!render_vp) {
            fwXformPop(node);
        } else {
        }
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
