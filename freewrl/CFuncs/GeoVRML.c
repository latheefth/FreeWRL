/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/
#include "Structs.h"
#include "GeoVRML.h"

void geoSystemCompile (struct Multi_String * geoSystem, int *__geoSystem, char *description) {
	int tmp;
	int numStrings;
	char *cptr;

	*__geoSystem = GEO_GD + GEO_WE;

	/* how many text strings in geoSystem field? */
	numStrings = geoSystem->n;

	/* Spacial Reference Frame */
	if (numStrings >=1) {
		cptr = SvPV(geoSystem->p[0],PL_na);
		if (strncmp("GD",cptr,2) == 0) *__geoSystem = GEO_GD;
		else if (strncmp("GC",cptr,2) == 0) *__geoSystem = GEO_GC;
		else if (strncmp("UTM",cptr,3) == 0) *__geoSystem = GEO_UTM;
		else if (strncmp("GDC",cptr,3) == 0) *__geoSystem = GEO_GD;
		else if (strncmp("GCC",cptr,3) == 0) *__geoSystem = GEO_GC;
		else printf ("Unknown Spatial Ref Frame :%s: found in :%s\n",
				cptr,description);
	}

	/* geoids or ellipsoid */
	if (numStrings >= 2) {
		cptr = SvPV(geoSystem->p[1],PL_na);
		if (strncmp("AA",cptr,2) == 0) *__geoSystem +=  GEO_AA;
		else if (strncmp("AM",cptr,2) == 0) *__geoSystem +=  GEO_AM;
		else if (strncmp("AN",cptr,2) == 0) *__geoSystem +=  GEO_AN;
		else if (strncmp("BN",cptr,2) == 0) *__geoSystem +=  GEO_BN;
		else if (strncmp("BR",cptr,2) == 0) *__geoSystem +=  GEO_BR;
		else if (strncmp("CC",cptr,2) == 0) *__geoSystem +=  GEO_CC;
		else if (strncmp("CD",cptr,2) == 0) *__geoSystem +=  GEO_CD;
		else if (strncmp("EA",cptr,2) == 0) *__geoSystem +=  GEO_EA;
		else if (strncmp("EB",cptr,2) == 0) *__geoSystem +=  GEO_EB;
		else if (strncmp("EC",cptr,2) == 0) *__geoSystem +=  GEO_EC;
		else if (strncmp("ED",cptr,2) == 0) *__geoSystem +=  GEO_ED;
		else if (strncmp("EE",cptr,2) == 0) *__geoSystem +=  GEO_EE;
		else if (strncmp("EF",cptr,2) == 0) *__geoSystem +=  GEO_EF;
		else if (strncmp("FA",cptr,2) == 0) *__geoSystem +=  GEO_FA;
		else if (strncmp("HE",cptr,2) == 0) *__geoSystem +=  GEO_HE;
		else if (strncmp("HO",cptr,2) == 0) *__geoSystem +=  GEO_HO;
		else if (strncmp("ID",cptr,2) == 0) *__geoSystem +=  GEO_ID;
		else if (strncmp("IN",cptr,2) == 0) *__geoSystem +=  GEO_IN;
		else if (strncmp("KA",cptr,2) == 0) *__geoSystem +=  GEO_KA;
		else if (strncmp("RF",cptr,2) == 0) *__geoSystem +=  GEO_RF;
		else if (strncmp("SA",cptr,2) == 0) *__geoSystem +=  GEO_SA;
		else if (strncmp("WD",cptr,2) == 0) *__geoSystem +=  GEO_WD;
		else if (strncmp("WE",cptr,2) == 0) *__geoSystem +=  GEO_WE;
		else if (strncmp("WGS84",cptr,5) == 0) *__geoSystem +=  GEO_WGS84;
		else {
			printf ("Unknown Geoid :%s: found in:%s\n",
				cptr,description);
			*__geoSystem += GEO_WE;
		}
	} else {
		*__geoSystem += GEO_WE;
	}
}


void render_GeoOrigin (struct VRML_GeoOrigin *node) {
        /* is the position "compiled" yet? */
        if (node->_change != node->_dlchange) {
                if (sscanf (SvPV(node->geoCoords,PL_na),"%f %f %f",&node->__geoCoords.c[0],
                        &node->__geoCoords.c[1], &node->__geoCoords.c[2]) != 3) {
                        printf ("GeoOrigin: invalid geoCoords string: :%s:\n",
                                        SvPV(node->geoCoords,PL_na));
                }

                geoSystemCompile (&node->geoSystem, &node->__geoSystem,"GeoOrigin");
                node->_dlchange = node->_change;
        }
}

void render_GeoLocation (struct VRML_GeoLocation *node) {
        /* is the position "compiled" yet? */
        if (node->_change != node->_dlchange) {
                if (sscanf (SvPV(node->geoCoords,PL_na),"%f %f %f",&node->__geoCoords.c[0],
                        &node->__geoCoords.c[1], &node->__geoCoords.c[2]) != 3) {
                        printf ("GeoLocation: invalid geoCoords string: :%s:\n",
                                        SvPV(node->geoCoords,PL_na));
                }

                geoSystemCompile (&node->geoSystem, &node->__geoSystem,"GeoLocation");
                node->_dlchange = node->_change;
        }
	printf ("GeoLocating to %f %f %f\n",node->__geoCoords.c[0],node->__geoCoords.c[1],
			node->__geoCoords.c[2]);
}
