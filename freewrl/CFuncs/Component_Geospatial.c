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



*********************************************************************/
#include "headers.h"
#include "Viewer.h"
#include "Bindable.h"
#include "Collision.h"
#include "Component_Geospatial.h"
#include "OpenGL_Utils.h"


/* defines used to get a SFVec3d into/outof a function that expects a MFVec3d */
#define MF_SF_TEMPS	struct Multi_Vec3d mIN; struct Multi_Vec3d  mOUT;
#define INIT_MF_FROM_SF(myNode, myField) \
	mIN.n = 1; \
	mIN.p = MALLOC(sizeof (struct SFVec3d)); \
	mIN.p[0].c[0] = myNode-> myField .c[0];\
	mIN.p[0].c[1] = myNode-> myField .c[1];\
	mIN.p[0].c[2] = myNode-> myField .c[2];\
	mOUT.n=0; mOUT.p = NULL;

#define MF_FIELD_IN_OUT &mIN, &mOUT
#define COPY_MF_TO_SF(myNode, myField) \
	myNode-> myField .c[0] = mOUT.p[0].c[0]; \
	myNode-> myField .c[1] = mOUT.p[0].c[1]; \
	myNode-> myField .c[2] = mOUT.p[0].c[2]; \
	FREE_IF_NZ(mIN.p); FREE_IF_NZ(mOUT.p);


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
	case typ: Gd_Gc(inCoords,outCoords,typ##_A, typ##_F); break;

#define UTM_ELLIPSOID(typ) \
	case typ: Utm_Gd (inCoords, tcp, typ##_A, typ##_F, geoSystem->p[3], geoSystem->p[2], TRUE); \
		  Gd_Gc(tcp,outCoords,typ##_A, typ##_F); break;

static int geoInit = FALSE;
/*static struct SFVec3d geoViewPointCenter = {(double)0.0, (double)0.0, (double)0.0}; */

static struct X3D_GeoOrigin *geoorigin = NULL;

static void compile_geoSystem (int nodeType, struct Multi_String *args, struct Multi_Int32 *srf);
static void moveCoords(struct Multi_Int32*, struct Multi_Vec3d *, struct Multi_Vec3d *);
static void Gd_Gc (struct Multi_Vec3d *, struct Multi_Vec3d *, double, double);


/* convert GD ellipsiod to GC coordinates */
static void Gd_Gc (struct Multi_Vec3d *inc, struct Multi_Vec3d *outc, double radius, double eccentricity) {
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
		printf ("Gd_Gc, ining %lf %lf %lf\n",LATITUDE_IN, LONGITUDE_IN, ELEVATION_IN);
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
		printf ("Gd_Gc, outing %lf %lf %lf\n", GC_X_OUT, GC_Y_OUT, GC_Z_OUT);
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
	if (!northing_first) { northing = 1; easting = 0;
	}
		
	#ifdef VERBOSE
	if (!hemisphere_north) {
		printf ("NOT hemisphere_north\n"); 
	} else { 
		printf ("hemisphere_north\n");
	}
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

		printf ("utmtogd\t%lf %lf %lf\n\t%lf %lf %lf\n", NORTHING_IN, EASTING_IN, ELEVATION_IN, LATITUDE_OUT, LONGITUDE_OUT, ELEVATION_IN);
		#endif

        } 
}

/* take a set of coords, and a geoSystem, and create a set of moved coords */
static void moveCoords (struct Multi_Int32* geoSystem, struct Multi_Vec3d *inCoords, struct Multi_Vec3d *outCoords) {
	struct Multi_Vec3d tmpCoords; /* used UTM conversions */
	struct Multi_Vec3d *tcp; /* used UTM conversions */

	int i;

	/* tmpCoords used for UTM coding */
	tmpCoords.n=0; tmpCoords.p=NULL;
	tcp = &tmpCoords;

	/* make sure the output has enough space for our converted data */
	ENSURE_SPACE(outCoords)

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
				ENSURE_SPACE(tcp)

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
				FREE_IF_NZ(tcp->p)
			break;
		default :
			printf ("incorrect geoSystem field, %s\n",stringGEOSPATIALType(geoSystem->p[0]));
			return;

	}
}

#define INITIALIZE_GEOSPATIAL \
	if (geoorigin == NULL) { \
		initializeGeospatial((struct X3D_GeoOrigin **) &node->geoOrigin); \
		/* printf ("initgeosp, node initialized %u (%s) has geoo %u\n",node, stringNodeType(node->_nodeType), node->geoOrigin); */ \
	} else if (X3D_GEOORIGIN(node->geoOrigin) != geoorigin) { \
		if (node->geoOrigin != NULL) ConsoleMessage ("have more than 1 GeoOrigin..."); \
		node->geoOrigin = geoorigin; /* make all same */ \
	}

static void initializeGeospatial (struct X3D_GeoOrigin **nodeptr)  {
	MF_SF_TEMPS

	#ifdef VERBOSE
	printf ("\ninitializing GeoSpatial code nodeptr %u, geoorigin %u\n",*nodeptr, geoorigin); 
	#endif

	if (*nodeptr != NULL) {
		if (X3D_GEOORIGIN(*nodeptr)->_nodeType != NODE_GeoOrigin) {
			printf ("expected a GeoOrigin node, but got a node of type %s\n",
				X3D_GEOORIGIN(*nodeptr)->_nodeType);
			geoorigin = createNewX3DNode(NODE_GeoOrigin); /* dummy node, because of error */
		} else {
			/* printf ("um, just setting geoorign to %u\n",(*nodeptr)); */
			geoorigin = X3D_GEOORIGIN(*nodeptr);
		}
	} else {
		printf ("expected a non-null geoOrigin, faking it\n");
		geoorigin = createNewX3DNode(NODE_GeoOrigin); /* dummy node, because none specified */
		*nodeptr = geoorigin;
	}

	compile_geoSystem (geoorigin->_nodeType, &geoorigin->geoSystem, &geoorigin->__geoSystem);
	INIT_MF_FROM_SF(geoorigin,geoCoords)
	moveCoords(&geoorigin->__geoSystem, MF_FIELD_IN_OUT);
	COPY_MF_TO_SF(geoorigin, __movedCoords)

	#ifdef VERBOSE
	printf ("initializeGeospatial, __movedCoords %lf %lf %lf, ryup %d, geoSystem %d %d %d %d\n",
		geoorigin->__movedCoords.c[0],
		geoorigin->__movedCoords.c[1],
		geoorigin->__movedCoords.c[2],
		geoorigin->rotateYUp,
		geoorigin->__geoSystem.p[0],
		geoorigin->__geoSystem.p[1],
		geoorigin->__geoSystem.p[2],
		geoorigin->__geoSystem.p[3]);
	printf ("initializeGeospatial, done\n\n");
	#endif
}


/* calculate a translation that moves a Geo node to local space */
static void GeoMove(struct X3D_GeoOrigin *geoOrigin, struct Multi_Int32* geoSystem, struct Multi_Vec3d *inCoords, struct Multi_Vec3d *outCoords) {
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

	for (i=0; i<outCoords->n; i++) {
		outCoords->p[i].c[0] = (double) 0.0; outCoords->p[i].c[1] = (double) 0.0; outCoords->p[i].c[2] = (double) 0.0;
	}

	#ifdef VERBOSE
	for (i=0; i<outCoords->n; i++) {
		printf ("start of GeoMove, inCoords %d: %lf %lf %lf\n",i, inCoords->p[i].c[0], inCoords->p[i].c[1], inCoords->p[i].c[2]);
	}
	#endif



	/* check the GeoOrigin attached node */
	if (geoOrigin != NULL) {
		if (X3D_GEOORIGIN(geoOrigin)->_nodeType != NODE_GeoOrigin) {
			ConsoleMessage ("GeoMove, expected a GeoOrigin, found a %s",stringNodeType(X3D_GEOORIGIN(geoOrigin)->_nodeType));
			printf ("GeoMove, expected a GeoOrigin, found a %s\n",stringNodeType(X3D_GEOORIGIN(geoOrigin)->_nodeType));
			return;
		}

		myOrigin = geoOrigin; /* local one */
	} else {
		myOrigin = geoorigin; /* global one */
	}
	/* printf ("GeoMove, using myOrigin %u, geoorigin %u, passed in geoOrigin %u with vals %lf %lf %lf\n",myOrigin, geoorigin, myOrigin,
		myOrigin->geoCoords.c[0], myOrigin->geoCoords.c[1], myOrigin->geoCoords.c[2] ); */
		

	moveCoords(geoSystem, inCoords, outCoords);

	for (i=0; i<outCoords->n; i++) {

	#ifdef VERBOSE
	printf ("GeoMove, before subtracting origin %lf %lf %lf\n", outCoords->p[i].c[0], outCoords->p[i].c[1], outCoords->p[i].c[2]);
	printf ("	... origin %lf %lf %lf\n",myOrigin->__movedCoords.c[0], myOrigin->__movedCoords.c[1], myOrigin->__movedCoords.c[2]);
	#endif

	outCoords->p[i].c[0] -= myOrigin->__movedCoords.c[0];
	outCoords->p[i].c[1] -= myOrigin->__movedCoords.c[1];
	outCoords->p[i].c[2] -= myOrigin->__movedCoords.c[2];

	#ifdef VERBOSE
	printf ("GeoMove, after subtracting origin %lf %lf %lf\n", outCoords->p[i].c[0], outCoords->p[i].c[1], outCoords->p[i].c[2]);
	#endif
	}
}

static void GeoOrient (struct SFVec3d locPos, struct DFRotation *orient) {

	Quaternion qx;
	Quaternion qz;
	Quaternion qr;

	#ifdef VERBOSE
	printf ("GeoOrient, calculating orient for %lf %lf %lf\n", locPos.c[0], locPos.c[1], locPos.c[2]);
	#endif


	/* initialie qx and qz */
	vrmlrot_to_quaternion (&qz,0.0, 0.0, 1.0, RADIANS_PER_DEGREE*((double)90.0 + locPos.c[1]));

	#ifdef VERBOSE 
	printf ("qz angle (deg) %lf angle (rad) %lf quat: %lf %lf %lf %lf\n",((double)90.0 + locPos.c[1]), 
		RADIANS_PER_DEGREE*((double)90.0 + locPos.c[1]),qz.x, qz.y, qz.z,qz.w);
	#endif

	vrmlrot_to_quaternion (&qx,1.0, 0.0, 0.0, RADIANS_PER_DEGREE*((double)180.0 - locPos.c[0]));

	#ifdef VERBOSE 
	printf ("qx angle (deg) %lf angle (rad) %lf quat: %lf %lf %lf %lf\n",
		((double)180.0 - locPos.c[0]), RADIANS_PER_DEGREE*((double)180.0 - locPos.c[0]), qx.x, qx.y, qx.z,qx.w);
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
			3:	"S" - value is FALSE, not S, value is TRUE */

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
			3:	"S" - value is FALSE, not S, value is TRUE */
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

	#ifdef VERBOSE
	printf ("compiling GeoLocation\n");
	#endif

	/* work out the position */
	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);
	INIT_MF_FROM_SF(node, geoCoords)
	GeoMove(X3D_GEOORIGIN(node->geoOrigin), &node->__geoSystem, MF_FIELD_IN_OUT);
	COPY_MF_TO_SF(node, __movedCoords)

	/* work out the local orientation */
	GeoOrient(node->__movedCoords, &node->__localOrient);

	#ifdef VERBOSE
	printf ("compile_GeoLocation, orig coords %lf %lf %lf, moved %lf %lf %lf\n", node->geoCoords.c[0], node->geoCoords.c[1], node->geoCoords.c[2], node->__movedCoords.c[0], node->__movedCoords.c[1], node->__movedCoords.c[2]);
	printf ("	rotation is %lf %lf %lf %lf\n",
			node->__localOrient.r[0],
			node->__localOrient.r[1],
			node->__localOrient.r[2],
			node->__localOrient.r[3]);
	#endif

	MARK_NODE_COMPILED

	#ifdef VERBOSE
	printf ("compiled GeoLocation\n\n");
	#endif
}

void compile_GeoLOD (struct X3D_GeoLOD * node) {
	#ifdef VERBOSE
	printf ("compiling GeoLOD\n");
	#endif

	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoMetadata (struct X3D_GeoMetadata * node) {
	#ifdef VERBOSE
	printf ("compiling GeoMetadata\n");

	#endif


	MARK_NODE_COMPILED
}

void compile_GeoOrigin (struct X3D_GeoOrigin * node) {
	#ifdef VERBOSE
	printf ("compiling GeoOrigin\n");
	#endif

	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoPositionInterpolator (struct X3D_GeoPositionInterpolator * node) {
	#ifdef VERBOSE
	printf ("compiling GeoPositionInterpolator\n");
	#endif

	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

void compile_GeoTouchSensor (struct X3D_GeoTouchSensor * node) {
	#ifdef VERBOSE
	printf ("compiling GeoTouchSensor\n");
	#endif

	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	MARK_NODE_COMPILED
}

/**************************************************************************/

void compile_GeoViewpoint (struct X3D_GeoViewpoint * node) {
	MF_SF_TEMPS

	#ifdef VERBOSE
	printf ("compileViewpoint is %u, its geoOrigin is %u (type %s) \n",node, node->geoOrigin, 
		stringNodeType(X3D_GEOORIGIN(node->geoOrigin)->_nodeType));
	#endif

	compile_geoSystem (node->_nodeType, &node->geoSystem, &node->__geoSystem);

	#ifdef VERBOSE
	printf ("GeoViewpoint, position at start of compile is %lf %lf %lf\n",node->position.c[0], node->position.c[1], node->position.c[2]);
	#endif

	INIT_MF_FROM_SF(node, position)


	GeoMove(X3D_GEOORIGIN(node->geoOrigin), &node->__geoSystem, MF_FIELD_IN_OUT);
	COPY_MF_TO_SF(node, __movedPosition)

	/* geoViewPointCenter.c[0] = -node->__movedPosition.c[0];
	geoViewPointCenter.c[1] = -node->__movedPosition.c[1];
	geoViewPointCenter.c[2] = -node->__movedPosition.c[2]; */

	MARK_NODE_COMPILED

	#ifdef VERBOSE
	printf ("compiled GeoViewpoint\n\n");
	#endif

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
	glRotated(-node->orientation.r[3]/PI*180.0,node->orientation.r[0],node->orientation.r[1],
		node->orientation.r[2]); 
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

void
bind_geoviewpoint (struct X3D_GeoViewpoint *node) {
	Quaternion q_i;

	INITIALIZE_GEOSPATIAL
	COMPILE_IF_REQUIRED

	/* set Viewer position and orientation */

	#ifdef VERBOSE
	printf ("bind_viewpoint, setting Viewer to %lf %lf %lf orient %f %f %f %f\n",node->__movedPosition.c[0],node->__movedPosition.c[1],
	node->__movedPosition.c[2],node->orientation.r[0],node->orientation.r[1],node->orientation.r[2],
	node->orientation.r[3]);
	printf ("	node %u fieldOfView %f\n",node,node->fieldOfView);
	#endif
	


	Viewer.Pos.x = node->__movedPosition.c[0];
	Viewer.Pos.y = node->__movedPosition.c[1];
	Viewer.Pos.z = node->__movedPosition.c[2];
	Viewer.AntiPos.x = node->__movedPosition.c[0];
	Viewer.AntiPos.y = node->__movedPosition.c[1];
	Viewer.AntiPos.z = node->__movedPosition.c[2];

	vrmlrot_to_quaternion (&Viewer.Quat,node->orientation.r[0],
		node->orientation.r[1],node->orientation.r[2],node->orientation.r[3]);

	vrmlrot_to_quaternion (&q_i,node->orientation.r[0],
		node->orientation.r[1],node->orientation.r[2],node->orientation.r[3]);
	inverse(&(Viewer.AntiQuat),&q_i);

	resolve_pos(&Viewer);
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
		double my_rotation;

                /* glPushMatrix();*/
		fwXformPush(node);

		/* TRANSLATION */

		glTranslated(node->__movedCoords.c[0], node->__movedCoords.c[1], node->__movedCoords.c[2]);
		/* glTranslated(node->__movedCoords.c[0]-geoViewPointCenter.c[0],
			node->__movedCoords.c[1]-geoViewPointCenter.c[1],
			node->__movedCoords.c[2]-geoViewPointCenter.c[2]); */
		/*
		printf ("prep_GeoLoc trans to %lf %lf %lf\n",node->__movedCoords.c[0],node->__movedCoords.c[1],node->__movedCoords.c[2]);
		printf ("          (really to %lf %lf %lf)\n",node->__movedCoords.c[0]-geoViewPointCenter.c[0],
			node->__movedCoords.c[1]-geoViewPointCenter.c[1],
			node->__movedCoords.c[2]-geoViewPointCenter.c[2]);
		*/

		my_rotation = node->__localOrient.r[3]/3.1415926536*180;
		glRotated(my_rotation, node->__localOrient.r[0],node->__localOrient.r[1],node->__localOrient.r[2]);

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

