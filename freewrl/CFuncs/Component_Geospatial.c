/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Geospatial Component


Coordinate Conversion algorithms were taken from 2 locations after
reading and comprehending the references. The code selected was
taken and modified, because the original coders "knew their stuff";
any problems with the modified code should be sent to John Stewart.

------
References:

Jean Meeus "Astronomical Algorithms", 2nd Edition, Chapter 11, page 82

"World Geodetic System"
http://en.wikipedia.org/wiki/WGS84
http://en.wikipedia.org/wiki/Geodetic_system#Conversion

"Mathworks Aerospace Blockset"
http://www.mathworks.com/access/helpdesk/help/toolbox/aeroblks/index.html?/access/helpdesk/help/toolbox/aeroblks/geocentrictogeodeticlatitude.html&http://www.google.ca/search?hl=en&q=Geodetic+to+Geocentric+conversion+equation&btnG=Google+Search&meta=

"TRANSFORMATION OF GEOCENTRIC TO GEODETIC COORDINATES WITHOUT APPROXIMATIONS"
http://www.astro.uni.torun.pl/~kb/Papers/ASS/Geod-ASS.htm

"Geodetic Coordinate Conversions"
http://www.gmat.unsw.edu.au/snap/gps/clynch_pdfs/coordcvt.pdf

"TerrestrialCoordinates.c"
http://www.lsc-group.phys.uwm.edu/lal/slug/nightly/doxygen/html/TerrestrialCoordinates_8c.html

------
Code Conversions:

UTM to Geodetic:
	Geo::Coordinates::UTM - Perl extension for Latitiude Longitude conversions.
	Copyright (c) 2000,2002,2004,2007 by Graham Crookham. All rights reserved.

Geodetic to Geocentric:
	Filename: Gdc_To_Gcc_Converter.java
	Author: Dan Toms, SRI International
	Package: GeoTransform <http://www.ai.sri.com/geotransform/>
	Acknowledgements:
	  The algorithms used in the package were created by Ralph Toms and
	  first appeared as part of the SEDRIS Coordinate Transformation API.
	  These were subsequently modified for this package. This package is
	  not part of the SEDRIS project, and the Java code written for this
	  package has not been certified or tested for correctness by NIMA.

Geocentric to Geodetic:
	same as Geodetic to Geocentric.


*********************************************************************/
#include "headers.h"
#include "Viewer.h"
#include "Bindable.h"
#include "Collision.h"
#include "Component_Geospatial.h"
#include "OpenGL_Utils.h"


#define MARK_META_EVENT(type) \
	/* we store oldmetadata as a SFTime because it =64 bits, and it will not cause problems \
	   on node garbage collection as it will not then be a duplicate SFNode pointer */ \
	if (node->__oldmetadata != node->metadata) { \
		MARK_EVENT((void*)node, offsetof (struct X3D_##type, metadata));  \
		node->__oldmetadata = node->metadata; \
	}

/* defines used to get a SFVec3d into/outof a function that expects a MFVec3d */
#define MF_SF_TEMPS	struct Multi_Vec3d mIN; struct Multi_Vec3d  mOUT; struct Multi_Vec3d gdCoords;
#define FREE_MF_SF_TEMPS FREE_IF_NZ(gdCoords.p); FREE_IF_NZ(mOUT.p);


#define INIT_MF_FROM_SF(myNode, myField) \
	mIN.n = 1; \
	mIN.p = MALLOC(sizeof (struct SFVec3d)); \
	mIN.p[0].c[0] = myNode-> myField .c[0];\
	mIN.p[0].c[1] = myNode-> myField .c[1];\
	mIN.p[0].c[2] = myNode-> myField .c[2];\
	mOUT.n=0; mOUT.p = NULL; \
	gdCoords.n=0; gdCoords.p = NULL;

#define MF_FIELD_IN_OUT &mIN, &mOUT, &gdCoords
#define COPY_MF_TO_SF(myNode, myField) \
	myNode-> myField .c[0] = mOUT.p[0].c[0]; \
	myNode-> myField .c[1] = mOUT.p[0].c[1]; \
	myNode-> myField .c[2] = mOUT.p[0].c[2]; \
	FREE_IF_NZ(mIN.p); FREE_IF_NZ(mOUT.p);


#define MOVE_TO_ORIGIN(me)	GeoMove(X3D_GEOORIGIN(me->geoOrigin), &me->__geoSystem, &mIN, &mOUT, &gdCoords);
#define COMPILE_GEOSYSTEM(me) compile_geoSystem (me->_nodeType, &me->geoSystem, &me->__geoSystem);

#define RADIANS_PER_DEGREE (double)0.0174532925199432957692
#define DEGREES_PER_RADIAN (double)57.2957795130823208768

#define ENSURE_SPACE(variableInQuestion) \
	/* enough room for output? */ \
	if (variableInQuestion ->n < inCoords->n) { \
		if (variableInQuestion ->p != NULL) { \
			FREE_IF_NZ(variableInQuestion->p); \
		} \
		variableInQuestion ->p = MALLOC(sizeof (struct SFVec3d) * inCoords->n); \
		variableInQuestion ->n = inCoords->n; \
	} 

/* for UTM, GC, GD conversions */
#define ELEVATION_OUT   outc->p[i].c[elevation]
#define ELEVATION_IN    inc->p[i].c[elevation]
#define EASTING_IN	inc->p[i].c[easting]
#define NORTHING_IN	inc->p[i].c[northing]
#define UTM_SCALE 	(double)0.9996
#define LATITUDE_OUT	outc->p[i].c[latitude]
#define LONGITUDE_OUT	outc->p[i].c[longitude]
#define LATITUDE_IN	inc->p[i].c[latitude]
#define LONGITUDE_IN	inc->p[i].c[longitude]
#define GC_X_OUT 	outc->p[i].c[0]
#define GC_Y_OUT 	outc->p[i].c[1]
#define GC_Z_OUT 	outc->p[i].c[2]

/* for Gd_Gc conversions */
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
	case typ: Gd_Gc(inCoords,outCoords,typ##_A, typ##_F,geoSystem->p[3]); break;

#define UTM_ELLIPSOID(typ) \
	case typ: Utm_Gd (inCoords, gdCoords, typ##_A, typ##_F, geoSystem->p[3], geoSystem->p[2], TRUE); \
		  Gd_Gc(gdCoords,outCoords,typ##_A, typ##_F, geoSystem->p[3]); break;

/* single SFVec3d pointers */
#define GCC_X gcc->c[0]
#define GCC_Y gcc->c[1]
#define GCC_Z gcc->c[2]
#define GDC_LAT gdc->c[0]
#define GDC_LON gdc->c[1]
#define GDC_ELE gdc->c[2]

#define INITIALIZE_GEOSPATIAL(me) \
	initializeGeospatial((struct X3D_GeoOrigin **) &me->geoOrigin); 

static int gcToGdInit = FALSE;
static int geoInit = FALSE;
/*static struct SFVec3d geoViewPointCenter = {(double)0.0, (double)0.0, (double)0.0}; */

/* static struct X3D_GeoOrigin *geoorigin = NULL; */

static void compile_geoSystem (int nodeType, struct Multi_String *args, struct Multi_Int32 *srf);
static void moveCoords(struct Multi_Int32*, struct Multi_Vec3d *, struct Multi_Vec3d *, struct Multi_Vec3d *);
static void Gd_Gc (struct Multi_Vec3d *, struct Multi_Vec3d *, double, double, int);
static void gccToGdc (struct SFVec3d *, struct SFVec3d *); 

/* for converting from GC to GD */
static double A, F, C, A2, C2, Eps2, Eps21, Eps25, C254, C2DA, CEE,
                 CE2, CEEps2, TwoCEE, tem, ARat1, ARat2, BRat1, BRat2, B1,B2,B3,B4,B5;


/* return the radius for the geosystem */
static double getEllipsoidRadius(int geosp) {
	switch (geosp) {
		case GEOSP_AA: return GEOSP_AA_A;
		case GEOSP_AM: return GEOSP_AM_A;
		case GEOSP_AN: return GEOSP_AN_A;
		case GEOSP_BN: return GEOSP_BN_A;
		case GEOSP_BR: return GEOSP_BR_A;
		case GEOSP_CC: return GEOSP_CC_A;
		case GEOSP_CD: return GEOSP_CD_A;
		case GEOSP_EA: return GEOSP_EA_A;
		case GEOSP_EB: return GEOSP_EB_A;
		case GEOSP_EC: return GEOSP_EC_A;
		case GEOSP_ED: return GEOSP_ED_A;
		case GEOSP_EE: return GEOSP_EE_A;
		case GEOSP_EF: return GEOSP_EF_A;
		case GEOSP_FA: return GEOSP_FA_A;
		case GEOSP_HE: return GEOSP_HE_A;
		case GEOSP_HO: return GEOSP_HO_A;
		case GEOSP_ID: return GEOSP_ID_A;
		case GEOSP_IN: return GEOSP_IN_A;
		case GEOSP_KA: return GEOSP_KA_A;
		case GEOSP_RF: return GEOSP_RF_A;
		case GEOSP_SA: return GEOSP_SA_A;
		case GEOSP_WD: return GEOSP_WD_A;
		case GEOSP_WE: return GEOSP_WE_A;

	}
	printf ("getEllipsoidRadius, not valid geosp, returning default\n");
	return GEOSP_WE_A; /* default value, as good as any, I guess */
}


/* move ourselves BACK to the from the GeoOrigin */
static void retractOrigin(struct X3D_GeoOrigin *myGeoOrigin, struct SFVec3d *gcCoords) {
	if (myGeoOrigin != NULL) {
		gcCoords->c[0] += myGeoOrigin->__movedCoords.c[0];
		gcCoords->c[1] += myGeoOrigin->__movedCoords.c[1];
		gcCoords->c[2] += myGeoOrigin->__movedCoords.c[2];
	}
}


/* convert GD ellipsiod to GC coordinates */
static void Gd_Gc (struct Multi_Vec3d *inc, struct Multi_Vec3d *outc, double radius, double eccentricity, int lat_first) {
	int i;
	double A = radius;
	double A2 = radius*radius;
	double F = (double)(1/eccentricity);
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

	if (!lat_first) {
		printf ("Gd_Gc, NOT lat first\n");
		latitude = 1; longitude = 0;
	}

	/* enough room for output? */
	if (outc->n < inc->n) {
		FREE_IF_NZ(outc->p);
		outc->p = MALLOC(sizeof (struct SFVec3d) * inc->n);
		outc->n = inc->n;
	}

	#ifdef VERBOSE
	printf ("Gd_Gc, have n of %d\n",inc->n);
	#endif

	for (i=0; i<inc->n; i++) {
		#ifdef VERBOSE
		printf ("Gd_Gc, ining lat %lf long %lf ele %lf   ",LATITUDE_IN, LONGITUDE_IN, ELEVATION_IN);
		#endif

		source_lat = RADIANS_PER_DEGREE * LATITUDE_IN;
		source_lon = RADIANS_PER_DEGREE * LONGITUDE_IN;
	
		#ifdef VERBOSE
		printf ("Source Latitude  %lf Source Longitude %lf\n",source_lat, source_lon);
		#endif

		slat = sin(source_lat);
		slat2 = slat*slat;
		clat = cos(source_lat);
	
		#ifdef VERBOSE
		printf ("slat %lf slat2 %lf clat %lf\n",slat, slat2, clat);
		#endif


		/* square root approximation for Rn */
		Rn = A / ( (.25 - Eps25 * slat2 + .9999944354799/4) + (.25-Eps25 * slat2)/(.25 - Eps25 * slat2 + .9999944354799/4));
	
		RnPh = Rn + ELEVATION_IN;

		#ifdef VERBOSE
		printf ("Rn %lf RnPh %lf\n",Rn, RnPh);
		#endif


		GC_X_OUT = RnPh * clat * cos(source_lon);
		GC_Y_OUT = RnPh * clat * sin(source_lon);
		GC_Z_OUT = ((C2 / A2) * Rn + ELEVATION_IN) * slat;

		#ifdef VERBOSE
		printf ("Gd_Gc, outing x %lf y %lf z %lf\n", GC_X_OUT, GC_Y_OUT, GC_Z_OUT);
		#endif
	}
}


/* convert UTM to GC coordinates by converting to GD as an intermediary step */
static void Utm_Gd (struct Multi_Vec3d *inc, struct Multi_Vec3d *outc, double radius, double flatten, int hemisphere_north, int zone, int northing_first) {
        double source_x, source_y,u,su,cu,su2,xlon0,temp,phi1,sp,sp2,cp,cp2,tp,tp2,eta2,top,rn,b3,b4,b5,b6,d1,d2;
	int i;
	int northing = 0;	/* for determining which input value is northing */
	int easting = 1;	/* for determining which input value is easting */
	int elevation = 2;	/* elevation is always third value, input AND output */
	int latitude = 0;	/* always return latitude as first value */
	int longitude = 1;	/* always return longtitude as second value */

	/* create the ERM constants. */
    	double A = radius;
	double F = 1.0/flatten;
	double C      = (A) * (1-F);
	double Eccentricity   = (F) * (2.0-F);

	double myEasting;
	double myphi1rad;
	double myphi1;
	double myN1;
	double myT1;
	double myC1;
	double myR1;
	double myD;
	double Latitude;
	double Longitude;
	double longitudeOrigin;
	double myeccPrimeSquared;
	double myNorthing;
	double northingDRCT1;
	double eccRoot;
	double calcConstantTerm1;
	double calcConstantTerm2;
	double calcConstantTerm3;
	double calcConstantTerm4;
	double calcConstantTerm5;

	/* is the values specified with an "easting_first?" */
	if (!northing_first) { northing = 1; easting = 0; }
if (!northing_first) printf ("UTM to GD, not northing first, flipping norhting and easting\n");
		
	#ifdef VERBOSE
	if (northing_first) printf ("northing first\n"); else printf ("NOT northing_first\n");
	if (!hemisphere_north) printf ("NOT hemisphere_north\n"); else printf ("hemisphere_north\n"); 
	#endif


	/* enough room for output? */
	if (outc->n < inc->n) {
		FREE_IF_NZ(outc->p);
		outc->p = MALLOC(sizeof (struct SFVec3d) * inc->n);
		outc->n = inc->n;
	}

	/* constants for all UTM vertices */
	longitudeOrigin = (zone -1) * 6 - 180 + 3;
	myeccPrimeSquared = Eccentricity/(((double) 1.0) - Eccentricity);
	eccRoot = (((double)1.0) - sqrt (((double)1.0) - Eccentricity))/
	       (((double)1.0) + sqrt (((double)1.0) - Eccentricity));

	calcConstantTerm1 = ((double)1.0) -Eccentricity/
		((double)4.0) - ((double)3.0) *Eccentricity*Eccentricity/
		((double)64.0) -((double)5.0) *Eccentricity*Eccentricity*Eccentricity/((double)256.0);

	calcConstantTerm2 = ((double)3.0) * eccRoot/((double)2.0) - ((double)27.0) *eccRoot*eccRoot*eccRoot/((double)32.0);
	calcConstantTerm3 = ((double)21.0) * eccRoot*eccRoot/ ((double)16.0) - ((double)55.0) *eccRoot*eccRoot*eccRoot*eccRoot/ ((double)32.0);
	calcConstantTerm4 = ((double)151.0) *eccRoot*eccRoot*eccRoot/ ((double)96.0);

	#ifdef VERBOSE
	printf ("zone %d\n",zone);
	printf ("longitudeOrigin %lf\n",longitudeOrigin);
	printf ("myeccPrimeSquared %lf\n",myeccPrimeSquared);
	printf ("eccRoot %lf\n",eccRoot);
	#endif

	/* go through each vertex specified */
        for(i=0;i<inc->n;i++) {
		/* get the values for THIS UTM vertex */
		ELEVATION_OUT = ELEVATION_IN;
		myEasting = EASTING_IN - 500000;
		if (hemisphere_north) myNorthing = NORTHING_IN;
		else myNorthing = NORTHING_IN - (double)10000000.0;

		#ifdef VERBOSE
		printf ("myEasting %lf\n",myEasting);
		printf ("myNorthing %lf\n",myNorthing);
		#endif


		/* scale the northing */
		myNorthing= myNorthing / UTM_SCALE;

		northingDRCT1 = myNorthing /(radius * calcConstantTerm1);

		myphi1rad = northingDRCT1 + 
			calcConstantTerm2 * sin(((double)2.0) *northingDRCT1)+
			calcConstantTerm3 * sin(((double)4.0) *northingDRCT1)+
			calcConstantTerm4 * sin(((double)6.0) *northingDRCT1);

		myN1 = radius/sqrt(((double)1.0) - Eccentricity * sin(myphi1rad) * sin (myphi1rad));
		myT1 = tan(myphi1rad) * tan(myphi1rad); 
		myC1 = Eccentricity * cos(myphi1rad) * cos (myphi1rad);
		myR1 = radius * (((double)1.0) - Eccentricity) / pow(((double)1.0) - Eccentricity * sin(myphi1rad) * sin (myphi1rad), 1.5);
		myD = myEasting/(myN1*UTM_SCALE);

		Latitude = myphi1rad-(myN1*tan(myphi1rad)/myR1)*
				(myD*myD/((double)2.0) -
			(((double)5.0) + ((double)3.0) *myT1+ ((double)10.0) *myC1-
			((double)4.0) *myC1*myC1- ((double)9.0) *myeccPrimeSquared)*
			
			myD*myD*myD*myD/((double)24.0) +(((double)61.0) +((double)90.0) *
			myT1+((double)298.0) *myC1+ ((double)45.0) *myT1*myT1-
			((double)252.0) * myeccPrimeSquared- ((double)3.0) *myC1*myC1)*myD*myD*myD*myD*myD*myD/((double)720.0));


		Longitude = (myD-(((double)1.0)+((double)2.0)*myT1+myC1)*myD*myD*myD/((double)6.0)+(((double)5.0) - ((double)2.0) *myC1+
			((double)28.0) *myT1-((double)3.0) *myC1*myC1+
			((double)8.0) *myeccPrimeSquared+((double)24.0) *myT1*myT1)*myD*myD*myD*myD*myD/120)/cos(myphi1rad);



		LATITUDE_OUT = Latitude * DEGREES_PER_RADIAN;
		LONGITUDE_OUT = longitudeOrigin + Longitude * DEGREES_PER_RADIAN;


		#ifdef VERBOSE
		printf ("myNorthing scaled %lf\n",myNorthing);
		printf ("northingDRCT1 %lf\n",northingDRCT1);
		printf ("myphi1rad %lf\n",myphi1rad);
		printf ("myN1 %lf\n",myN1);
		printf ("myT1 %lf\n",myT1);
		printf ("myC1 %lf\n",myC1);
		printf ("myR1 %lf\n",myR1);
		printf ("myD %lf\n",myD);
		printf ("latitude %lf\n",Latitude);
		printf ("longitude %lf\n",Longitude);

		printf ("utmtogd\tnorthing %lf easting %lf ele %lf\n\tlat %lf long %lf ele %lf\n", NORTHING_IN, EASTING_IN, ELEVATION_IN, LATITUDE_OUT, LONGITUDE_OUT, ELEVATION_IN);
		#endif

        } 
}

/* take a set of coords, and a geoSystem, and create a set of moved coords */
/* we keep around the GD coords because we need them for rotation calculations */
/* parameters: 
	geoSystem:	compiled geoSystem integer array pointer
	inCoords:	coordinate structure for input coordinates, ANY coordinate type
	outCoords:	area for GC coordinates. Will MALLOC size if required 
	gdCoords:	GD coordinates, used for rotation calculations in later stages. WILL MALLOC THIS */

static void moveCoords (struct Multi_Int32* geoSystem, struct Multi_Vec3d *inCoords, struct Multi_Vec3d *outCoords, struct Multi_Vec3d *gdCoords) {

	int i;

	/* tmpCoords used for UTM coding */
	gdCoords->n=0; gdCoords->p=NULL;

	/* make sure the output has enough space for our converted data */
	ENSURE_SPACE(outCoords)
	ENSURE_SPACE(gdCoords)

	/* GD Geosystem - copy coordinates, and convert them to GC */
	switch (geoSystem->p[0]) {
		case  GEOSP_GD:
				/* GD_Gd_Gc_convert (inCoords, outCoords); */
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
					default: printf ("unknown Gd_Gc: %s\n", stringGEOSPATIALType(geoSystem->p[1]));
				}

				/* now, for the GD coord return values; is this in the correct format for calculating 
				   rotations? */
				gdCoords->n = inCoords->n;

				/* is the GD value NOT the WGS84 ellipsoid? */
				if (geoSystem->p[1] != GEOSP_WE) {
					/*no, convert BACK from the GC to GD, WGS84 level for the gd value returns */
					for (i=0; i<outCoords->n; i++) {
						gccToGdc (&outCoords->p[i], &gdCoords->p[i]);
					}
				} else {
					/* just copy the coordinates for the GD temporary return  */
					memcpy (gdCoords->p, inCoords->p, sizeof (struct SFVec3d) * inCoords->n);
				}
			break;
		case GEOSP_GC:
			/* an earth-fixed geocentric coord; no conversion required for gc value returns */
			for (i=0; i< inCoords->n; i++) {
				outCoords->p[i].c[0] = inCoords->p[i].c[0];
				outCoords->p[i].c[1] = inCoords->p[i].c[1];
				outCoords->p[i].c[2] = inCoords->p[i].c[2];

				/* convert this coord from GC to GD, WGS84 ellipsoid for gd value returns */
				gccToGdc (&inCoords->p[i], &gdCoords->p[i]);
			}

			break;
		case GEOSP_UTM:
				/* GD coords will be returned from the conversion process....*/
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
					default: printf ("unknown Gd_Gc: %s\n", stringGEOSPATIALType(geoSystem->p[1]));
				}
			break;
		default :
			printf ("incorrect geoSystem field, %s\n",stringGEOSPATIALType(geoSystem->p[0]));
			return;

	}
}


static void initializeGeospatial (struct X3D_GeoOrigin **nodeptr)  {
	MF_SF_TEMPS
	struct X3D_GeoOrigin *node = NULL;

	#ifdef VERBOSE
	printf ("\ninitializing GeoSpatial code nodeptr %u\n",*nodeptr); 
	#endif

	if (*nodeptr != NULL) {
		if (X3D_GEOORIGIN(*nodeptr)->_nodeType != NODE_GeoOrigin) {
			printf ("expected a GeoOrigin node, but got a node of type %s\n",
				X3D_GEOORIGIN(*nodeptr)->_nodeType);
			*nodeptr = NULL;
			return;
		} else {
			/* printf ("um, just setting geoorign to %u\n",(*nodeptr)); */
			node = X3D_GEOORIGIN(*nodeptr);
		}

		/* printf ("initGeoSpatial ich %d ch %d\n",node->_ichange, node->_change); */

		if NODE_NEEDS_COMPILING {
			compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);
			INIT_MF_FROM_SF(node,geoCoords)
			moveCoords(&node->__geoSystem, MF_FIELD_IN_OUT);
			COPY_MF_TO_SF(node, __movedCoords)
	
			#ifdef VERBOSE
			printf ("initializeGeospatial, __movedCoords %lf %lf %lf, ryup %d, geoSystem %d %d %d %d\n",
				node->__movedCoords.c[0],
				node->__movedCoords.c[1],
				node->__movedCoords.c[2],
				node->rotateYUp,
				node->__geoSystem.p[0],
				node->__geoSystem.p[1],
				node->__geoSystem.p[2],
				node->__geoSystem.p[3]);
			printf ("initializeGeospatial, done\n\n");
			#endif
			FREE_MF_SF_TEMPS
			MARK_NODE_COMPILED
		}
	}
}


/* calculate a translation that moves a Geo node to local space */
static void GeoMove(struct X3D_GeoOrigin *geoOrigin, struct Multi_Int32* geoSystem, struct Multi_Vec3d *inCoords, struct Multi_Vec3d *outCoords,
		struct Multi_Vec3d *gdCoords) {
	int i;
	struct X3D_GeoOrigin * myOrigin;

	#ifdef VERBOSE
	printf ("\nstart of GeoMove... %d coords\n",inCoords->n);
	#endif

	/* enough room for output? */
	if (inCoords->n==0) {return;}
	if (outCoords->n < inCoords->n) {
		if (outCoords->n!=0) {
			FREE_IF_NZ(outCoords->p);
		}
		outCoords->p = MALLOC(sizeof (struct SFVec3d) * inCoords->n);
		outCoords->n = inCoords->n;
	}

	/* set out values to 0.0 for now */
	for (i=0; i<outCoords->n; i++) {
		outCoords->p[i].c[0] = (double) 0.0; outCoords->p[i].c[1] = (double) 0.0; outCoords->p[i].c[2] = (double) 0.0;
	}

	#ifdef VERBOSE
	for (i=0; i<outCoords->n; i++) {
		printf ("start of GeoMove, inCoords %d: %lf %lf %lf\n",i, inCoords->p[i].c[0], inCoords->p[i].c[1], inCoords->p[i].c[2]);
	}
	#endif



	/* check the GeoOrigin attached node */
	myOrigin = NULL;
	if (geoOrigin != NULL) {
		if (X3D_GEOORIGIN(geoOrigin)->_nodeType != NODE_GeoOrigin) {
			ConsoleMessage ("GeoMove, expected a GeoOrigin, found a %s",stringNodeType(X3D_GEOORIGIN(geoOrigin)->_nodeType));
			printf ("GeoMove, expected a GeoOrigin, found a %s\n",stringNodeType(X3D_GEOORIGIN(geoOrigin)->_nodeType));
			return;
		}

		myOrigin = geoOrigin; /* local one */
	}
	/* printf ("GeoMove, using myOrigin %u, passed in geoOrigin %u with vals %lf %lf %lf\n",myOrigin, myOrigin,
		myOrigin->geoCoords.c[0], myOrigin->geoCoords.c[1], myOrigin->geoCoords.c[2] ); */
		

	moveCoords(geoSystem, inCoords, outCoords, gdCoords);

	for (i=0; i<outCoords->n; i++) {

	#ifdef VERBOSE
	printf ("GeoMove, before subtracting origin %lf %lf %lf\n", outCoords->p[i].c[0], outCoords->p[i].c[1], outCoords->p[i].c[2]);
	if (myOrigin != NULL) printf ("	... origin %lf %lf %lf\n",myOrigin->__movedCoords.c[0], myOrigin->__movedCoords.c[1], myOrigin->__movedCoords.c[2]);
	#endif

	if (myOrigin != NULL) {
		outCoords->p[i].c[0] -= myOrigin->__movedCoords.c[0];
		outCoords->p[i].c[1] -= myOrigin->__movedCoords.c[1];
		outCoords->p[i].c[2] -= myOrigin->__movedCoords.c[2];
	}

	#ifdef VERBOSE
	printf ("GeoMove, after subtracting origin %lf %lf %lf\n", outCoords->p[i].c[0], outCoords->p[i].c[1], outCoords->p[i].c[2]);
	#endif
	}
}


/* for converting BACK to GD from GC */
static void initializeGcToGdParams(void) {
        double polx2b,polx3b,polx4b,polx5b;
 
        A = GEOSP_WE_A;
        F = GEOSP_WE_F;
            
        /*  Create the ERM constants. */
        A2     = A * A;
        F      =1/(F);
        C      =(A) * (1-F);
        C2     = C * C;
        Eps2   =(F) * (2.0-F);
        Eps21  =Eps2 - 1.0;
        Eps25  =.25 * (Eps2);
        C254   =54.0 * C2;        
        
        C2DA   = C2 / A;
        CE2    = A2 - C2;
        tem    = CE2 / C2;
        CEE    = Eps2 * Eps2;        
        TwoCEE =2.0 * CEE;
        CEEps2 =Eps2 * CE2;
         
        /* UPPER BOUNDS ON POINT */
     

        ARat1  =pow((A + 50005.0),2);
        ARat2  =(ARat1) / pow((C+50005.0),2);
    
        /* LOWER BOUNDS ON POINT */
        
        BRat1  =pow((A-10005.0),2);
        BRat2  =(BRat1) / pow((C-10005.0),2);
          
	/* use WE ellipsoid */
	B1=0.100225438677758E+01;
	B2=-0.393246903633930E-04;
	B3=0.241216653453483E+12;
	B4=0.133733602228679E+14;
	B5=0.984537701867943E+00;
	gcToGdInit = TRUE;
}


/* convert BACK to a GD coordinate, from GC coordinates using WE ellipsoid */
static void gccToGdc (struct SFVec3d *gcc, struct SFVec3d *gdc) {
        double w2,w,z2,testu,testb,top,top2,rr,q,s12,rnn,s1,zp2,wp,wp2,cf,gee,alpha,cl,arg2,p,xarg,r2,r1,ro,
               arg0,s,roe,arg,v,zo;

	#ifdef VERBOSE
	printf ("gccToGdc input %lf %lf %lf\n",GCC_X, GCC_Y, GCC_Z);
	#endif
	

	if (!gcToGdInit) initializeGcToGdParams();

        /* CHECK FOR SPECIAL CASES*/
            if (!(GCC_X == 0.0)) ; /* null statement */
            else {
                if (GCC_Y > 0.0) 
                    GDC_LAT = PI/2;
                else {
                    if (GCC_Y < 0.0)
                        GDC_LON = -PI/2;
                    else {
                        if (GCC_Z > 0) {
                            GDC_LAT = PI/2;
                            GDC_LON = 0.0;
                            GDC_ELE = GCC_Z;
                            return;
                        } else {
                            if (GCC_Z < 0.0) {
                                GDC_LAT = -PI/2;
                                GDC_LON   =  0.0;
                                GDC_ELE   =  GCC_Z;
                            
                                return;
                            } else {
                            	GDC_LAT = 0.0;
                            	GDC_LON = 0.0;
                            	GDC_ELE = 0.0;
                            	return;
                        }
                    }
                }
            }
        }

	/* printf ("gccToGdc, past special cases\n"); */

        /* END OF SPECIAL CASES */

        w2=GCC_X * GCC_X + GCC_Y * GCC_Y;
        w=sqrt(w2);
        z2=GCC_Z * GCC_Z;

        testu=w2 + ARat2 * z2;
        testb=w2 + BRat2 * z2;

	/* printf ("w2 %lf w %lf z2 %lf testu %lf testb %lf\n",w2,w,z2,testu,testb); */

        if ((testb > BRat1) && (testu < ARat1)) 
        {    

            /*POINT IS BETWEEN-10 KIL AND 50 KIL, SO COMPUTE TANGENT LATITUDE */
    
            top= GCC_Z * (B1 + (B2 * w2 + B3) /
                 (B4 + w2 * B5 + z2));

            top2=top*top;

            rr=top2+w2;
                  
            q=sqrt(rr);
                  
            /* ****************************************************************
                  
               COMPUTE H IN LINE SQUARE ROOT OF 1-EPS2*SIN*SIN.  USE SHORT BINOMIAL
               EXPANSION PLUS ONE ITERATION OF NEWTON'S METHOD FOR SQUARE ROOTS.
            */

            s12=top2/rr;

            rnn = A / ( (.25 - Eps25*s12 + .9999944354799/4) + (.25-Eps25*s12)/(.25 - Eps25*s12 + .9999944354799/4));
            s1=top/q;
        
            /******************************************************************/

            /* TEST FOR H NEAR POLE.  if SIN(¯)**2 <= SIN(45.)**2 THEN NOT NEAR A POLE.*/  
    
            if (s12 < .50)
                GDC_ELE = q-rnn;
            else
                GDC_ELE = GCC_Z / s1 + (Eps21 * rnn);
                GDC_LAT = atan(top / w);
                GDC_LON = atan2(GCC_Y,GCC_X);
        }
              /* POINT ABOVE 50 KILOMETERS OR BELOW -10 KILOMETERS  */
        else /* Do Exact Solution  ************ */
        { 
            wp2=GCC_X * GCC_X + GCC_Y * GCC_Y;
            zp2=GCC_Z * GCC_Z;
            wp=sqrt(wp2);
            cf=C254 * zp2;
            gee=wp2 - (Eps21 * zp2) - CEEps2;
            alpha=cf / (gee*gee);
            cl=CEE * wp2 * alpha / gee;
            arg2=cl * (cl + 2.0);
            s1=1.0 + cl + sqrt(arg2);
            s=pow(s1,(1.0/3.0));
            p=alpha / (3.0 * pow(( s + (1.0/s) + 1.0),2));
            xarg= 1.0 + (TwoCEE * p);
            q=sqrt(xarg);
            r2= -p * (2.0 * (1.0 - Eps2) * zp2 / ( q * ( 1.0 + q) ) + wp2);
            r1=(1.0 + (1.0 / q));
            r2 /=A2;

            /*    DUE TO PRECISION ERRORS THE ARGUMENT MAY BECOME NEGATIVE IF SO SET THE ARGUMENT TO ZERO.*/

            if (r1+r2 > 0.0)
                ro = A * sqrt( .50 * (r1+r2));
            else
                ro=0.0;

            ro=ro - p * Eps2 * wp / ( 1.0 + q);
            arg0 = pow(( wp - Eps2 * ro),2) + zp2;
            roe = Eps2 * ro;
            arg = pow(( wp - roe),2) + zp2;
            v=sqrt(arg - Eps2 * zp2);
            zo=C2DA * GCC_Z / v;
            GDC_ELE = sqrt(arg) * (1.0 - C2DA / v);
            top=GCC_Z+ tem*zo;
            GDC_LAT = atan( top / wp );
            GDC_LON =atan2(GCC_Y,GCC_X);
        }  /* end of Exact solution */

        GDC_LAT *= DEGREES_PER_RADIAN;
        GDC_LON *= DEGREES_PER_RADIAN;
}

/* calculate the rotation needed to apply to this position on the GC coordinate location */
static void GeoOrient (struct SFVec3d *gdCoords, struct DFRotation *orient) {
	Quaternion qx;
	Quaternion qz;
	Quaternion qr;

	/* initialie qx and qz */
	vrmlrot_to_quaternion (&qz,0.0, 0.0, 1.0, RADIANS_PER_DEGREE*((double)90.0 + gdCoords->c[1]));

	#ifdef VERBOSE 
	printf ("qz angle (deg) %lf angle (rad) %lf quat: %lf %lf %lf %lf\n",((double)90.0 + gdCoords->c[1]), 
		RADIANS_PER_DEGREE*((double)90.0 + gdCoords->c[1]),qz.x, qz.y, qz.z,qz.w);
	#endif

	vrmlrot_to_quaternion (&qx,1.0, 0.0, 0.0, RADIANS_PER_DEGREE*((double)180.0 - gdCoords->c[0]));

	#ifdef VERBOSE 
	printf ("qx angle (deg) %lf angle (rad) %lf quat: %lf %lf %lf %lf\n",
		((double)180.0 - gdCoords->c[0]), RADIANS_PER_DEGREE*((double)180.0 - gdCoords->c[0]), qx.x, qx.y, qx.z,qx.w);
	#endif

	add (&qr, &qx, &qz);

	#ifdef VERBOSE
	printf ("qr %lf %lf %lf %lf\n",qr.x, qr.y, qr.z,qr.w);
	#endif

        quaternion_to_vrmlrot(&qr, &orient->r[0], &orient->r[1], &orient->r[2], &orient->r[3]);

	#ifdef VERBOSE
	printf ("rotation %lf %lf %lf %lf\n",orient->r[0], orient->r[1], orient->r[2], orient->r[3]);
	#endif

}


/* compileGeosystem - encode the return value such that srf->p[x] is...
			0:	spatial reference frame	(GEOSP_UTM, GEOSP_GC, GEOSP_GD);
			1:	spatial coordinates (defaults to GEOSP_WE)
			2:	UTM zone number, 1..60. ID_UNDEFINED = not specified
			3:	UTM:	if "S" - value is FALSE, not S, value is TRUE 
				GD:	if "latitude_first" TRUE, if "longitude_first", FALSE
				GC:	if "northing_first" TRUE, if "easting_first", FALSE */

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
	srf->p[3] = TRUE;

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
		/* possible parameter: GC:	if "northing_first" TRUE, if "easting_first", FALSE */
		srf->p[1] = ID_UNDEFINED;
		for (i=0; i<args->n; i++) {
			if (strcmp("northing_first",args->p[i]->strptr) == 0) { srf->p[3] = TRUE;
			} else if (strcmp("easting_first",args->p[i]->strptr) == 0) { srf->p[3] = FALSE;
			} else if (i!=this_srf_ind) ConsoleMessage ("geoSystem GC parameter %s not allowed geospatial coordinates",args->p[i]->strptr);
		}
	} else if (this_srf == GEOSP_GD) {
		indexT this_p_ind;

		srf->p[1] = GEOSP_WE;
		/* possible parameters: ellipsoid, gets put into element 1.
				if "latitude_first" TRUE, if "longitude_first", FALSE */

		/* is there an optional argument? */
		for (i=0; i<args->n; i++) {
			/* printf ("geosp_gd, ind %d i am %d string %s\n",i, this_srf_ind,args->p[i]->strptr); */
                        if (strcmp("latitude_first", args->p[i]->strptr) == 0) {
				srf->p[3] = TRUE;
                        } else if (strcmp("longitude_first", args->p[i]->strptr) == 0) {
				srf->p[3] = FALSE;
			} else {
				if (i!= this_srf_ind) {
					indexT tc = findFieldInGEOSPATIAL(args->p[i]->strptr);
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
				}
			}
		}
	} else {
		/* this must be UTM */
		/* encode the return value such that srf->p[x] is...
			0:	spatial reference frame	(GEOSP_UTM, GEOSP_GC, GEOSP_GD);
			1:	spatial coordinates (defaults to GEOSP_WE)
			2:	UTM zone number, 1..60. ID_UNDEFINED = not specified
			3:	UTM:	if "S" - value is FALSE, not S, value is TRUE  */
		/* first go through, and find the Spatial Reference Frame, GD, UTM, or GC */
		for (i=0; i<args->n; i++) {
			if (i != this_srf_ind) {
				if (strcmp ("S",args->p[i]->strptr) == 0) {
					srf->p[3] = FALSE;
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
							ConsoleMessage("expected valid UTM Ellipsoid parameter in node %s",stringNodeType(nodeType));
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

/************************************************************************/
void compile_GeoCoordinate (struct X3D_GeoCoordinate * node) {
	MF_SF_TEMPS
	int i;

	#ifdef VERBOSE
	printf ("compiling GeoCoordinate\n");
	#endif

	/* standard MACROS expect specific field names */
	mIN = node->point;
	mOUT.p = NULL; mOUT.n = 0;


	INITIALIZE_GEOSPATIAL(node)
	COMPILE_GEOSYSTEM(node)
	MOVE_TO_ORIGIN(node)

	/* convert the doubles down to floats, because coords are used as floats in FreeWRL. */
	FREE_IF_NZ(node->__movedCoords.p);
	node->__movedCoords.p = MALLOC (sizeof (struct SFColor)  * mOUT.n);
	for (i=0; i<mOUT.n; i++) {
		node->__movedCoords.p[i].c[0] = (float) mOUT.p[i].c[0];
		node->__movedCoords.p[i].c[1] = (float) mOUT.p[i].c[1];
		node->__movedCoords.p[i].c[2] = (float) mOUT.p[i].c[2];
		#ifdef VERBOSE
		printf ("coord %d now is %f %f %f\n", i, node->__movedCoords.p[i].c[0],node->__movedCoords.p[i].c[1],node->__movedCoords.p[i].c[2]);
		#endif
	}
	node->__movedCoords.n = mOUT.n;

	FREE_IF_NZ(gdCoords.p);
	FREE_IF_NZ(mOUT.p);
	MARK_NODE_COMPILED
	
	/* events */
	MARK_META_EVENT(GeoOrigin)

}


/***********************************************************************/
/* check validity of ElevationGrid fields */
int checkX3DGeoElevationGridFields (struct X3D_ElevationGrid *node, float **points, int *npoints) {
	MF_SF_TEMPS
	int i,j;
	int nx;
	double xSp;
	int nz;
	double zSp;
	double *height;
	int ntri;
	int nh;
	struct X3D_PolyRep *rep;
	struct X3D_GeoElevationGrid *parent; /* this ElevationGrid is the child of a GeoElevationGrid */
	float *newpoints;
	double newPoint[3];
	int nquads;
	int *cindexptr;
	float *tcoord;
	double myHeightAboveEllip = 0.0;
	int mySRF = 0;
	
	/* ok, a GeoElevationGrid has an ElevationGrid for a child; the ElevationGrid does all the
	   rendering, colliding, etc, etc, as it has coords in local coord system. The GeoElevationGrid
	   contains the user source. */

	if (node->_nparents<1) {
		printf ("checkX3DGeoElevationGrids - no parent?? \n");
		return FALSE;
	}

	parent = node->_parents[0]; /* this ElevationGrid is the child of a GeoElevationGrid */

	/* get these from the GeoElevationGrid */
	nx = parent->xDimension;
	xSp = parent->xSpacing;
	nz = parent->zDimension;
	zSp = parent->zSpacing;
	height = parent->height.p;
	nh = parent->height.n;

	/* various values for converting to GD/UTM, etc */
	if (parent->__geoSystem.n != 0)  {
		mySRF = parent->__geoSystem.p[0];
		myHeightAboveEllip = getEllipsoidRadius(parent->__geoSystem.p[1]);
	}

	rep = (struct X3D_PolyRep *)node->_intern;

	/* work out how many triangles/quads we will have */
	ntri = (nx && nz ? 2 * (nx-1) * (nz-1) : 0);
	nquads = ntri/2;

	/* check validity of input fields */
	if(nh != nx * nz) {
		if (nh > nx * nz) {
			printf ("GeoElevationgrid: warning: x,y vs. height: %d * %d ne %d:\n", nx,nz,nh);
		} else {
			printf ("GeoElevationgrid: error: x,y vs. height: %d * %d ne %d:\n", nx,nz,nh);
			return FALSE;
		}
	}

	/* do we have any triangles? */
	if ((nx < 2) || (nz < 2)) {
		printf ("GeoElevationGrid: xDimension and zDimension less than 2 %d %d\n", nx,nz);
		return FALSE;
	}

	/* any texture coordinates passed in? if so, DO NOT generate any texture coords here. */
        if (!(node->texCoord)) {
		/* allocate memory for texture coords */
		FREE_IF_NZ(rep->GeneratedTexCoords);

		/* 6 vertices per quad each vertex has a 2-float tex coord mapping */
		tcoord = rep->GeneratedTexCoords = (float *)MALLOC (sizeof (float) * nquads * 12); 

		rep->tcindex=0; /* we will generate our own mapping */
	}

	/* make up points array */
	/* a point is a vertex and consists of 3 floats (x,y,z) */
	newpoints = (float *)MALLOC (sizeof (float) * nz * nx * 3);
	 
	FREE_IF_NZ(rep->actualCoord);
	rep->actualCoord = (float *)newpoints;

	/* make up coord index */
	if (node->coordIndex.n > 0) FREE_IF_NZ(node->coordIndex.p);
	node->coordIndex.p = MALLOC (sizeof(int) * nquads * 5);
	cindexptr = node->coordIndex.p;

	node->coordIndex.n = nquads * 5;
	/* return the newpoints array to the caller */
	*points = newpoints;
	*npoints = node->coordIndex.n;

	#ifdef VERBOSE
	printf ("coordindex:\n");
	#endif

	for (j = 0; j < (nz -1); j++) {
		for (i=0; i < (nx-1) ; i++) {
			#ifdef VERBOSE
			printf ("	%d %d %d %d %d\n", j*nx+i, j*nx+i+nx, j*nx+i+nx+1, j*nx+i+1, -1);
			#endif
			
			*cindexptr = j*nx+i; cindexptr++;
			*cindexptr = j*nx+i+nx; cindexptr++;
			*cindexptr = j*nx+i+nx+1; cindexptr++;
			*cindexptr = j*nx+i+1; cindexptr++;
			*cindexptr = -1; cindexptr++;

		}
	}

	/* tex coords These need to be streamed now; that means for each quad, each vertex needs its tex coords. */
	/* if the texCoord node exists, let render_TextureCoordinate (or whatever the node is) do our work for us */
	if (!(node->texCoord)) {
		for (j = 0; j < (nz -1); j++) {
			for (i=0; i < (nx-1) ; i++) {
				/* first triangle, 3 vertexes */
				/* first tri */
				*tcoord = ((float) (i+0)/(nx-1)); tcoord++;
				*tcoord = ((float)(j+0)/(nz-1)); tcoord ++; 
			
				*tcoord = ((float) (i+0)/(nx-1)); tcoord++;
				*tcoord = ((float)(j+1)/(nz-1)); tcoord ++; 
	
				*tcoord = ((float) (i+1)/(nx-1)); tcoord++;
				*tcoord = ((float)(j+1)/(nz-1)); tcoord ++; 
	
				/* second tri */
				*tcoord = ((float) (i+0)/(nx-1)); tcoord++;
				*tcoord = ((float)(j+0)/(nz-1)); tcoord ++; 
	
				*tcoord = ((float) (i+1)/(nx-1)); tcoord++;
				*tcoord = ((float)(j+1)/(nz-1)); tcoord ++; 
	
				*tcoord = ((float) (i+1)/(nx-1)); tcoord++;
				*tcoord = ((float)(j+0)/(nz-1)); tcoord ++; 
			}
		}
	}
			
	/* Render_Polyrep will use this number of triangles */
	rep->ntri = ntri;

	/* pass these common fields from the parent GeoElevationGrid to the child
	   ElevationGrid. We must ensure that we do not free the copied SFNode
	   pointers when this node is destroyed */
	node->color = parent->color;
	node->normal = parent->normal;
	node->texCoord = parent->texCoord;
	node->ccw = !parent->ccw; /* NOTE THE FLIP HERE */
	node->colorPerVertex = parent->colorPerVertex;
	node->creaseAngle = (float) parent->creaseAngle;
	node->normalPerVertex = parent->normalPerVertex;
	node->solid = parent->solid;


	/* initialize arrays used for passing values into/out of the MOVE_TO_ORIGIN(node) values */
	mIN.n = nx * nz; 
	mIN.p = (struct SFVec3d *)MALLOC (sizeof (struct SFVec3d) * mIN.n);

        mOUT.n=0; mOUT.p = NULL;
        gdCoords.n=0; gdCoords.p = NULL;

	/* make up a series of points, then go and convert them to local coords */
	for (j=0; j<nz; j++) {
		for (i=0; i < nx; i++) {
		
			#ifdef VERBOSE
		 	printf ("		%lf %lf %lf # (hei ind %d) point [%d, %d]\n",
				xSp * i,
				height[i+(j*nx)] * ((double)parent->yScale),
				zSp * j,
				i+(j*nx), i,j);
			#endif
		
		
			/* Make up a new vertex. Add the geoGridOrigin to every point */

			if ((mySRF == GEOSP_GD) || (mySRF == GEOSP_UTM)) {
				/* GD - give it to em in Latitude/Longitude/Elevation order */
				/* UTM- or give it to em in Northing/Easting/Elevation order */
				/* latitude - range of -90 to +90 */
				mIN.p[i+(j*nx)].c[0] = zSp * j + parent->geoGridOrigin.c[0]; 
	
				/* longitude - range -180 to +180, or 0 to 360 */
				mIN.p[i+(j*nx)].c[1] =xSp * i + parent->geoGridOrigin.c[1];
	
				/* elevation, above geoid */
				mIN.p[i+(j*nx)].c[2] = (height[i+(j*nx)] *(parent->yScale)) + parent->geoGridOrigin.c[2]
					+ myHeightAboveEllip; 
			} else {
				/* nothing quite specified here - what do we really do??? */
				mIN.p[i+(j*nx)].c[0] = zSp * j + parent->geoGridOrigin.c[0]; 
	
				mIN.p[i+(j*nx)].c[1] =xSp * i + parent->geoGridOrigin.c[1];
	
				mIN.p[i+(j*nx)].c[2] = (height[i+(j*nx)] *(parent->yScale)) + parent->geoGridOrigin.c[2]
					+ myHeightAboveEllip; 

			}
		}
	}

	#ifdef VERBOSE
	printf ("points before moving origin:\n");
	for (j=0; j<nz; j++) {
		for (i=0; i < nx; i++) {
			printf ("	%lf %lf %lf # lat/long/height before MOVE, index %d\n",mIN.p[i+(j*nx)].c[0],
				mIN.p[i+(j*nx)].c[1],mIN.p[i+(j*nx)].c[2],i+(j*nx));

		}
	}
	#endif

	/* convert this point to a local coordinate */
        MOVE_TO_ORIGIN(parent)

	/* copy the resulting array back to the ElevationGrid */

	#ifdef VERBOSE
	printf ("points:\n");
	#endif

	for (j=0; j<nz; j++) {
		for (i=0; i < nx; i++) {
		/* copy this coordinate into our ElevationGrid array */
		newpoints[0] = (float) mOUT.p[i+(j*nx)].c[0];
		newpoints[1] = (float) mOUT.p[i+(j*nx)].c[1];
		newpoints[2] = (float) mOUT.p[i+(j*nx)].c[2];

		#ifdef VERBOSE
		printf ("	%f %f %f # converted, index %d\n",newpoints[0],newpoints[1],newpoints[2],i+(j*nx));
		#endif

		newpoints += 3;
		}
	}
	FREE_MF_SF_TEMPS

	return TRUE;
}


/************************************************************************/
/* GeoElevationGrid							*/
/************************************************************************/

/* a GeoElevationGrid creates a "real" elevationGrid node as a child for rendering. */
void compile_GeoElevationGrid (struct X3D_GeoElevationGrid * node) {

	#ifdef VERBOSE
	printf ("compiling GeoElevationGrid\n");
	#endif

	INITIALIZE_GEOSPATIAL(node)
	COMPILE_GEOSYSTEM(node)
	MARK_NODE_COMPILED
	
	/* events */
	MARK_META_EVENT(GeoOrigin)

}

void render_GeoElevationGrid (struct X3D_GeoElevationGrid *innode) {
	struct X3D_ElevationGrid *node = innode->__realElevationGrid;

	INITIALIZE_GEOSPATIAL(innode)
	COMPILE_IF_REQUIRED2(innode) /* same as COMPILE_IF_REQUIRED, but with innode */

	if (node == NULL) {	
		/* create an ElevationGrid so that we can render this. Actual vertex
		   data will be created via the checkX3DGeoElevationGrid function */
		node = createNewX3DNode(NODE_ElevationGrid);
		innode->__realElevationGrid =  node;
		ADD_PARENT(X3D_NODE(innode->__realElevationGrid), X3D_NODE(innode));
	} 
	COMPILE_POLY_IF_REQUIRED (NULL, node->color, node->normal, node->texCoord)
	CULL_FACE(node->solid)
	render_polyrep(node);
}


/* deref real ElevationGrid pointer */
void make_GeoElevationGrid (struct X3D_GeoElevationGrid *innode) {
printf ("make_GeoElevationGrid called\n");
}

/* deref real ElevationGrid pointer */
void collide_GeoElevationGrid (struct X3D_GeoElevationGrid *innode) {
	struct X3D_ElevationGrid *node = innode->__realElevationGrid;
	if (node != NULL) collide_IndexedFaceSet ((struct X3D_IndexedFaceSet *)node);
}

/* deref real ElevationGrid pointer */
void rendray_GeoElevationGrid (struct X3D_GeoElevationGrid *innode) {
	struct X3D_ElevationGrid *node = innode->__realElevationGrid;
	if (node != NULL) rendray_IndexedFaceSet ((struct X3D_IndexedFaceSet *)node);
}


/************************************************************************/
/* GeoLocation								*/
/************************************************************************/

void compile_GeoLocation (struct X3D_GeoLocation * node) {
	MF_SF_TEMPS

	#ifdef VERBOSE
	printf ("compiling GeoLocation\n");
	#endif

	/* work out the position */
	INITIALIZE_GEOSPATIAL(node)
	COMPILE_GEOSYSTEM(node)
	INIT_MF_FROM_SF(node, geoCoords)
	MOVE_TO_ORIGIN(node)
	COPY_MF_TO_SF(node, __movedCoords)

	/* work out the local orientation */
	GeoOrient(&gdCoords.p[0], &node->__localOrient);

	#ifdef VERBOSE
	printf ("compile_GeoLocation, orig coords %lf %lf %lf, moved %lf %lf %lf\n", node->geoCoords.c[0], node->geoCoords.c[1], node->geoCoords.c[2], node->__movedCoords.c[0], node->__movedCoords.c[1], node->__movedCoords.c[2]);
	printf ("	rotation is %lf %lf %lf %lf\n",
			node->__localOrient.r[0],
			node->__localOrient.r[1],
			node->__localOrient.r[2],
			node->__localOrient.r[3]);
	#endif

	MARK_NODE_COMPILED
	FREE_MF_SF_TEMPS
	
	/* events */
	MARK_META_EVENT(GeoOrigin)


	#ifdef VERBOSE
	printf ("compiled GeoLocation\n\n");
	#endif
}

void child_GeoLocation (struct X3D_GeoLocation *node) {
	int nc = (node->children).n;
	INITIALIZE_GEOSPATIAL(node)
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

	INITIALIZE_GEOSPATIAL(node)
	COMPILE_IF_REQUIRED
	INITIALIZE_EXTENT

}

/* do transforms, calculate the distance */
void prep_GeoLocation (struct X3D_GeoLocation *node) {
	GLfloat my_rotation;
	GLfloat my_scaleO=0;
	INITIALIZE_GEOSPATIAL(node)
	COMPILE_IF_REQUIRED

        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	/* printf ("prep_GeoLocation, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

	/* do we have any geometry visible, and are we doing anything with geometry? */
	OCCLUSIONTEST

	if(!render_vp) {
		double my_rotation;

                /* glPushMatrix();*/
		fwXformPush(node);

		/* TRANSLATION */

		glTranslated(node->__movedCoords.c[0], node->__movedCoords.c[1], node->__movedCoords.c[2]);
		/*
		printf ("prep_GeoLoc trans to %lf %lf %lf\n",node->__movedCoords.c[0],node->__movedCoords.c[1],node->__movedCoords.c[2]);
		printf ("          (really to %lf %lf %lf)\n",node->__movedCoords.c[0]-geoViewPointCenter.c[0],
			node->__movedCoords.c[1]-geoViewPointCenter.c[1],
			node->__movedCoords.c[2]-geoViewPointCenter.c[2]);
		*/

		my_rotation = node->__localOrient.r[3]/3.1415926536*180;
		glRotated(my_rotation, node->__localOrient.r[0],node->__localOrient.r[1],node->__localOrient.r[2]);

		/* did either we or the Viewpoint move since last time? */
		RECORD_DISTANCE
        }
}
void fin_GeoLocation (struct X3D_GeoLocation *node) {
	INITIALIZE_GEOSPATIAL(node)
	COMPILE_IF_REQUIRED


	OCCLUSIONTEST

        if(!render_vp) {
            fwXformPop(node);
        } else {
        }
}

/************************************************************************/
/* GeoLOD								*/
/************************************************************************/

#define LOAD_CHILD(childNode,childUrl) \
		/* create new inline node, link it in */ \
		if (node->childNode == NULL) node->childNode = createNewX3DNode(NODE_Inline); \
		ADD_PARENT(node->childNode, node); \
 \
		/* copy over the URL from parent */ \
		X3D_INLINE(node->childNode)->url.n = node->childUrl.n; \
		if (node->childUrl.n > 0) { \
			X3D_INLINE(node->childNode)->url.p = MALLOC(sizeof(struct Uni_String)*node->childUrl.n); \
			for (i=0; i<node->childUrl.n; i++) { \
				printf ("copying over url %s\n",node->childUrl.p[i]->strptr); \
				X3D_INLINE(node->childNode)->url.p[i] = newASCIIString(node->childUrl.p[i]->strptr); \
			} \
			X3D_INLINE(node->childNode)->load = TRUE; \
\
			/* now, get this file */ \
                	loadInline(X3D_INLINE(node->childNode)); \
		}  




void GeoLODchildren (struct X3D_GeoLOD *node) {
	int load = !(node->__outOfRange);
        struct X3D_Inline *inl;
	int i;


        /* lets see if we still have to load this one... */
        if (((node->__childloadstatus)==0) && (load)) {
		LOAD_CHILD(__child1Node,child1Url)
		LOAD_CHILD(__child2Node,child2Url)
		LOAD_CHILD(__child3Node,child3Url)
		LOAD_CHILD(__child4Node,child4Url)
                node->__childloadstatus = 1;
        } else if (!(load) && ((node->__childloadstatus) != 0)) {
                printf ("GeoLODloadChildren, removing children\n");
/*
                node->children.n = 0;
                FREE_IF_NZ (node->children.p);
*/
                node->__childloadstatus = 0;
        }
}


void compile_GeoLOD (struct X3D_GeoLOD * node) {
	MF_SF_TEMPS

	#ifdef VERBOSE
	printf ("compiling GeoLOD\n");
	#endif
	printf ("compiling GeoLOD %u\n",node);

	/* work out the position */
	INITIALIZE_GEOSPATIAL(node)
	COMPILE_GEOSYSTEM(node)
	INIT_MF_FROM_SF(node, center)
	MOVE_TO_ORIGIN(node)
	COPY_MF_TO_SF(node, __movedCoords)

	/* work out the local orientation */
	/* GeoOrient(&gdCoords.p[0], &node->__localOrient); */

	#ifdef VERBOSE
	printf ("compile_GeoLOD, orig coords %lf %lf %lf, moved %lf %lf %lf\n", node->center.c[0], node->center.c[1], node->center.c[2], node->__movedCoords.c[0], node->__movedCoords.c[1], node->__movedCoords.c[2]);

	printf ("children.n %d childurl 1: %u 2: %u 3: %u 4: %u rootUrl: %u rootNode: %d\n",
	node->children,
	node->child1Url,
	node->child2Url,
	node->child3Url,
	node->child4Url,
	node->rootUrl,
	node->rootNode.n);
	#endif

	MARK_NODE_COMPILED
	FREE_MF_SF_TEMPS
	
	/* events */
	MARK_META_EVENT(GeoOrigin)


	#ifdef VERBOSE
	printf ("compiled GeoLOD\n\n");
	#endif
}


void child_GeoLOD (struct X3D_GeoLOD *node) {
	INITIALIZE_GEOSPATIAL(node)
	COMPILE_IF_REQUIRED

        GLdouble mod[16];
        GLdouble proj[16];
        struct point_XYZ vec;
        double dist;
        int i;
	int oldOutOfRange = node->__outOfRange;

	/* printf ("child_GeoLOD, render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);  */

        /* calculate which one to display - only do this once per eventloop. */
        if (render_geom && (!render_blend)) {
                fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
                /* printf ("LOD, mat %lf %lf %lf\n",mod[12],mod[13],mod[14]);  */
                fwGetDoublev(GL_PROJECTION_MATRIX, proj);
                gluUnProject(0,0,0,mod,proj,viewport, &vec.x,&vec.y,&vec.z);

		#ifdef VERBOSE
		printf ("before cent move %lf %lf %lf ",vec.x, vec.y, vec.z);
                #endif

		vec.x -= (node->center).c[0];
                vec.y -= (node->center).c[1];
                vec.z -= (node->center).c[2];
		
		#ifdef VERBOSE
		printf ("after %lf %lf %lf\n ",vec.x, vec.y, vec.z);
		#endif

                dist = sqrt(VECSQ(vec));
                i = 0;

		#ifdef VERBOSE
		printf ("child GeoLOD, dist %lf rootNode %u rangeDif %lf \n",dist,node->rootNode.n,
			node->range);
		#endif

		/*printf ("calculating, dist %lf, node->range %f planes %lf %lf\n",dist,node->range, nearPlane, farPlane); */
		/* save this calculation */
		node->__outOfRange = (dist> node->range);

        }

	#ifdef VERBOSE
	if ( node->__outOfRange) {
		printf ("GeoLOD %u farther away\n",node);
	} else {
		printf ("GeoLOD %u closer\n",node);
	}
	#endif


	/* if we are out of range, use the rootNode or rootUrl field 	*/
	/* else, use the child1Url through the child4Url fields 	*/
	if (node->__outOfRange) {
		if (node->rootNode.n != 0)  {
			for (i=0; i<node->rootNode.n; i++) {
				render_node (node->rootNode.p[i]);
			}	
		} else if (node->rootUrl.n != 0) {
			/* try and load the root from the rootUrl */
			LOAD_CHILD(__rootUrl, rootUrl)
		}
	} else {
		/* go through 4 kids */
		/* has the outOfRange flag changed? */
		if (oldOutOfRange != node->__outOfRange) {
			GeoLODchildren (node);
		}

		/* render these children */
		if (node->__child1Node != NULL) render_node (node->__child1Node);
		if (node->__child2Node != NULL) render_node (node->__child2Node);
		if (node->__child3Node != NULL) render_node (node->__child3Node);
		if (node->__child4Node != NULL) render_node (node->__child4Node);
	}
}

/************************************************************************/
/* GeoMetaData								*/
/************************************************************************/

void compile_GeoMetadata (struct X3D_GeoMetadata * node) {
	#ifdef VERBOSE
	printf ("compiling GeoMetadata\n");

	#endif

	MARK_NODE_COMPILED
}

/************************************************************************/
/* GeoOrigin								*/
/************************************************************************/

void compile_GeoOrigin (struct X3D_GeoOrigin * node) {
	#ifdef VERBOSE
	printf ("compiling GeoOrigin\n");
	#endif

	/* INITIALIZE_GEOSPATIAL */
	COMPILE_GEOSYSTEM(node)
	MARK_NODE_COMPILED

	/* events */
	MARK_META_EVENT(GeoOrigin)

	if ((!APPROX(node->geoCoords.c[0],node->__oldgeoCoords.c[0])) ||
	   (!APPROX(node->geoCoords.c[1],node->__oldgeoCoords.c[1])) ||
	   (!APPROX(node->geoCoords.c[2],node->__oldgeoCoords.c[2]))) {
		MARK_EVENT(X3D_NODE(node), offsetof (struct X3D_GeoOrigin, geoCoords)); 
		memcpy (&node->__oldgeoCoords, &node->geoCoords, sizeof (struct SFVec3d));
	}
}

/************************************************************************/
/* GeoPositionInterpolator						*/
/************************************************************************/

void compile_GeoPositionInterpolator (struct X3D_GeoPositionInterpolator * node) {
	MF_SF_TEMPS
	int i;

	#ifdef VERBOSE
	printf ("compiling GeoPositionInterpolator\n");
	#endif

	/* standard MACROS expect specific field names */
	mIN = node->keyValue;
	mOUT.p = NULL; mOUT.n = 0;


	INITIALIZE_GEOSPATIAL(node)
	COMPILE_GEOSYSTEM(node)
	MOVE_TO_ORIGIN(node)

	
	/* keep the output values of this process */
	FREE_IF_NZ(node->__movedValue.p);
	node->__movedValue.p = mOUT.p;
	node->__movedValue.n = mOUT.n;

	FREE_IF_NZ(gdCoords.p);
	MARK_NODE_COMPILED
	
	/* events */
	MARK_META_EVENT(GeoOrigin)

}

/************************************************************************/
/* GeoTouchSensor							*/
/************************************************************************/

void compile_GeoTouchSensor (struct X3D_GeoTouchSensor * node) {
	#ifdef VERBOSE
	printf ("compiling GeoTouchSensor\n");
	#endif

	INITIALIZE_GEOSPATIAL(node)
	COMPILE_GEOSYSTEM(node)
	MARK_NODE_COMPILED
	
	/* events */
	MARK_META_EVENT(GeoOrigin)

}

void do_GeoTouchSensor ( void *ptr, int ev, int but1, int over) {
	struct X3D_GeoTouchSensor *node = (struct X3D_GeoTouchSensor *)ptr;

	COMPILE_IF_REQUIRED
        /* remember to POSSIBLE_PROTO_EXPANSION(node->geoOrigin, tmpN) */
	printf ("do_GeoTouchSensor\n");
        
}; 


/************************************************************************/
/* GeoViewpoint								*/
/************************************************************************/

void compile_GeoViewpoint (struct X3D_GeoViewpoint * node) {
	struct DFRotation localOrient;
	struct DFRotation orient;
	int i;
	Quaternion localQuat;
	Quaternion relQuat;
	Quaternion combQuat;

	MF_SF_TEMPS

	#ifdef VERBOSE
	printf ("compileViewpoint is %u, its geoOrigin is %u \n",node, node->geoOrigin);
	if (node->geoOrigin!=NULL) printf ("type %s\n",stringNodeType(X3D_GEOORIGIN(node->geoOrigin)->_nodeType));
	#endif

	/* work out the position */
	INITIALIZE_GEOSPATIAL(node)
	COMPILE_GEOSYSTEM(node)
	INIT_MF_FROM_SF(node, position)
	MOVE_TO_ORIGIN(node)
	COPY_MF_TO_SF(node, __movedPosition)


	/* work out the local orientation and copy doubles to floats */
	GeoOrient(&gdCoords.p[0], &localOrient);

	/* Quaternize the local Geospatial quaternion, and the specified rotation from the GeoViewpoint orientation field */
	vrmlrot_to_quaternion (&localQuat, localOrient.r[0], localOrient.r[1], localOrient.r[2], localOrient.r[3]);
	vrmlrot_to_quaternion (&relQuat, node->orientation.r[0], node->orientation.r[1], node->orientation.r[2], node->orientation.r[3]);

	/* add these together */
        add (&combQuat, &relQuat, &localQuat);

	/* get the rotation; 2 steps to convert doubles to floats;
           should be quaternion_to_vrmlrot(&combQuat, &node->__movedOrientation.r[0]... */
        quaternion_to_vrmlrot(&combQuat, &orient.r[0], &orient.r[1], &orient.r[2], &orient.r[3]);
	for (i=0; i<4; i++) node->__movedOrientation.r[i] = (float) orient.r[i];

        #ifdef VERBOSE
	printf ("compile_GeoViewpoint, final position %lf %lf %lf\n",node->__movedPosition.c[0],
		node->__movedPosition.c[1], node->__movedPosition.c[2]);

	printf ("compile_GeoViewpoint, getLocalOrientation %lf %lf %lf %lf\n",localOrient.r[0],
		localOrient.r[1], localOrient.r[2], localOrient.r[3]);
	printf ("compile_GeoViewpoint, initial orientation: %lf %lf %lf %lf\n",node->orientation.r[0],
		node->orientation.r[1], node->orientation.r[2], node->orientation.r[3]);
	printf ("compile_GeoViewpoint, final rotation %lf %lf %lf %lf\n",node->__movedOrientation.r[0], 
		node->__movedOrientation.r[1], node->__movedOrientation.r[2], node->__movedOrientation.r[3]);
	printf ("compile_GeoViewpoint, elevation from the WGS84 ellipsoid is %lf\n",gdCoords.p[0].c[2]);
        #endif

	MARK_NODE_COMPILED
	FREE_MF_SF_TEMPS
	
	/* events */
	MARK_META_EVENT(GeoOrigin)


	#ifdef VERBOSE
	printf ("compiled GeoViewpoint\n\n");
	#endif
}


void prep_GeoViewpoint (struct X3D_GeoViewpoint *node) {
	double a1;

	if (!render_vp) return;

	INITIALIZE_GEOSPATIAL(node)

	 /* printf ("RVP, node %d ib %d sb %d gepvp\n",node,node->isBound,node->set_bind);
	 printf ("VP stack %d tos %d\n",viewpoint_tos, viewpoint_stack[viewpoint_tos]);
	 */

	/* check the set_bind eventin to see if it is TRUE or FALSE */
	/* code to perform binding is now in set_viewpoint. */

	if(!node->isBound) return;
	COMPILE_IF_REQUIRED


	/* perform GeoViewpoint translations */
	glRotated(-node->__movedOrientation.r[3]/PI*180.0,node->__movedOrientation.r[0],node->__movedOrientation.r[1],
		node->__movedOrientation.r[2]); 
	glTranslated(-node->__movedPosition.c[0],-node->__movedPosition.c[1],-node->__movedPosition.c[2]);

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

/* GeoViewpoint speeds and avatar sizes are depenent on elevation above WGS_84. These are calculated here */
/* this is called from the Viewer functions */
float viewer_calculate_speed() {
	struct SFVec3d gcCoords;
	struct SFVec3d gdCoords;

	/* the current position is the GC coordinate */
	gcCoords.c[0]= Viewer.Pos.x;
	gcCoords.c[1] = Viewer.Pos.y;
	gcCoords.c[2] = Viewer.Pos.z;

	if (Viewer.GeoSpatialNode != NULL) {
        	retractOrigin(Viewer.GeoSpatialNode->geoOrigin, &gcCoords);
	}

        #ifdef VERBOSE
        printf ("viewer_calculate_speed, retracted %lf %lf %lf\n", gcCoords.c[0], gcCoords.c[1], gcCoords.c[2]);
        #endif

        /* convert from local (gc) to gd coordinates, using WGS84 ellipsoid */
        gccToGdc (&gcCoords, &gdCoords);

	/* printf ("speed is calculated from height %lf\n",gdCoords.c[2]); */

	/* speed is dependent on elevation above WGS84 ellipsoid */
	#define speed_scale 1.0
	Viewer.speed = fabs(gdCoords.c[2]/10.0 * Viewer.GeoSpatialNode->speedFactor);

	#ifdef VERBOSE
	printf ("speed is %lf\n",Viewer.speed); 
	#endif

	/* set the navigation info - use the GeoVRML algorithms */
	naviinfo.width = Viewer.speed*0.25;
	naviinfo.height = Viewer.speed*1.6;
	naviinfo.step = Viewer.speed*0.25;

}



void bind_geoviewpoint (struct X3D_GeoViewpoint *node) {
	Quaternion q_i;

	INITIALIZE_GEOSPATIAL(node)
	COMPILE_IF_REQUIRED

	/* set Viewer position and orientation */

	#ifdef VERBOSE
	printf ("bind_viewpoint, setting Viewer to %lf %lf %lf orient %f %f %f %f\n",node->__movedPosition.c[0],node->__movedPosition.c[1],
	node->__movedPosition.c[2],node->orientation.r[0],node->orientation.r[1],node->orientation.r[2],
	node->orientation.r[3]);
	printf ("	node %u fieldOfView %f\n",node,node->fieldOfView);
	#endif
	
	Viewer.GeoSpatialNode = node;

	Viewer.Pos.x = node->__movedPosition.c[0];
	Viewer.Pos.y = node->__movedPosition.c[1];
	Viewer.Pos.z = node->__movedPosition.c[2];
	Viewer.AntiPos.x = node->__movedPosition.c[0];
	Viewer.AntiPos.y = node->__movedPosition.c[1];
	Viewer.AntiPos.z = node->__movedPosition.c[2];

	vrmlrot_to_quaternion (&Viewer.Quat,node->__movedOrientation.r[0],
		node->__movedOrientation.r[1],node->__movedOrientation.r[2],node->__movedOrientation.r[3]);

	vrmlrot_to_quaternion (&q_i,node->__movedOrientation.r[0],
		node->__movedOrientation.r[1],node->__movedOrientation.r[2],node->__movedOrientation.r[3]);
	inverse(&(Viewer.AntiQuat),&q_i);

	resolve_pos(&Viewer);
}

