/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "Structs.h"
#include "GeoVRML.h"

#include "headers.h"

int GeoVerbose = 0;

int GeoSys     = GEO_GD + GEO_WE;	// which GeoSystem is parsed from the last GeoOrigin
double GeoOrig[3];			// last GeoOrigin parsed in lat/long/elvation format

/* Function Prototypes */
void parse_ellipsoid(int *dest, char *str, char *description);



/* look for an ellipsoid in the GeoSystem field */
void parse_ellipsoid(int *dest, char *str, char *description) {
	if (strncmp("AA",str,2) == 0) *dest +=  GEO_AA;
	else if (strncmp("AM",str,2) == 0) *dest +=  GEO_AM;
	else if (strncmp("AN",str,2) == 0) *dest +=  GEO_AN;
	else if (strncmp("BN",str,2) == 0) *dest +=  GEO_BN;
	else if (strncmp("BR",str,2) == 0) *dest +=  GEO_BR;
	else if (strncmp("CC",str,2) == 0) *dest +=  GEO_CC;
	else if (strncmp("CD",str,2) == 0) *dest +=  GEO_CD;
	else if (strncmp("EA",str,2) == 0) *dest +=  GEO_EA;
	else if (strncmp("EB",str,2) == 0) *dest +=  GEO_EB;
	else if (strncmp("EC",str,2) == 0) *dest +=  GEO_EC;
	else if (strncmp("ED",str,2) == 0) *dest +=  GEO_ED;
	else if (strncmp("EE",str,2) == 0) *dest +=  GEO_EE;
	else if (strncmp("EF",str,2) == 0) *dest +=  GEO_EF;
	else if (strncmp("FA",str,2) == 0) *dest +=  GEO_FA;
	else if (strncmp("HE",str,2) == 0) *dest +=  GEO_HE;
	else if (strncmp("HO",str,2) == 0) *dest +=  GEO_HO;
	else if (strncmp("ID",str,2) == 0) *dest +=  GEO_ID;
	else if (strncmp("IN",str,2) == 0) *dest +=  GEO_IN;
	else if (strncmp("KA",str,2) == 0) *dest +=  GEO_KA;
	else if (strncmp("RF",str,2) == 0) *dest +=  GEO_RF;
	else if (strncmp("SA",str,2) == 0) *dest +=  GEO_SA;
	else if (strncmp("WD",str,2) == 0) *dest +=  GEO_WD;
	else if (strncmp("WE",str,2) == 0) *dest +=  GEO_WE;
	else if (strncmp("WGS84",str,5) == 0) *dest +=  GEO_WGS84;
	else {
		printf ("Unknown ellipsoid :%s: found in:%s\n",
			str,description);
		*dest += GEO_WE;
	}
}


/* take a geoSystem field, and put it into a format that we can use internally */
void geoSystemCompile (struct Multi_String * geoSystem, int *__geoSystem, char *description) {
	int numStrings;
	char *cptr;
	int tmp, tz, sl;
	int xx;

	*__geoSystem = GEO_GD + GEO_WE;

	/* how many text strings in geoSystem field? */
	numStrings = geoSystem->n;

	/* Spacial Reference Frame */
	if (numStrings >=1) {
		cptr = SvPV(geoSystem->p[0],xx);
		if (strncmp("GD",cptr,2) == 0) *__geoSystem = GEO_GD;
		else if (strncmp("GC",cptr,2) == 0) *__geoSystem = GEO_GC;
		else if (strncmp("UTM",cptr,3) == 0) *__geoSystem = GEO_UTM;
		else if (strncmp("GDC",cptr,3) == 0) *__geoSystem = GEO_GD;
		else if (strncmp("GCC",cptr,3) == 0) *__geoSystem = GEO_GC;
		else printf ("Unknown Spatial Ref Frame :%s: found in :%s\n",
				cptr,description);
	}

	/* further parameters */
	if (*__geoSystem == GEO_GD) {
		/* GEO_GD geoids or ellipsoid */
		if (numStrings >= 2) {
			parse_ellipsoid (__geoSystem, SvPV(geoSystem->p[1],xx), description);
		} else {
			*__geoSystem += GEO_WE;
		}
	} else if (*__geoSystem == GEO_UTM) {
		for (tmp = 1; tmp < numStrings; tmp++) {
			cptr = SvPV(geoSystem->p[tmp],sl);
			if (cptr[0] == 'Z') {
				sscanf (cptr,"Z%d",&tz);
				if ((tz>60) || (tz<1)) {
					printf ("UTM Zone %s invalid in %s\n",cptr,description);
					tz = 1;
				}
				*__geoSystem += tz*GEO_UTM_ZONE_BASE;
			} else if ((cptr[0]=='S') && (sl==1)) {
				*__geoSystem += GEO_UTM_S_FLAG;
			} else parse_ellipsoid (__geoSystem,cptr,description);
		}
	}
}


void render_GeoOrigin (struct VRML_GeoOrigin *node) {
	int xx;
        /* is the position "compiled" yet? */
        if (node->_change != node->_dlchange) {
                if (sscanf (SvPV(node->geoCoords,xx),"%lf %lf %lf",&GeoOrig[0],
                        &GeoOrig[1], &GeoOrig[2]) != 3) {
                        printf ("GeoOrigin: invalid geoCoords string: :%s:\n",
                                        SvPV(node->geoCoords,xx));
                }

                geoSystemCompile (&node->geoSystem, &GeoSys,"GeoOrigin");

		if (GeoVerbose) printf ("GeoOrigin - lat %f long %f elev %f\n",
			GeoOrig[0],GeoOrig[1],GeoOrig[2]);

                node->_dlchange = node->_change;
        }
}

void render_GeoLocation (struct VRML_GeoLocation *node) {
	int xx;
	GLdouble modelMatrix[16];

        /* is the position "compiled" yet? */
        if (node->_change != node->_dlchange) {
                if (sscanf (SvPV(node->geoCoords,xx),"%f %f %f",&node->__geoCoords.c[0],
                        &node->__geoCoords.c[1], &node->__geoCoords.c[2]) != 3) {
                        printf ("GeoLocation: invalid geoCoords string: :%s:\n",
                                        SvPV(node->geoCoords,xx));
                }

                geoSystemCompile (&node->geoSystem, &node->__geoSystem,"GeoLocation");
                node->_dlchange = node->_change;
        }

	/* this is a transform */
	//fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	//printf ("modelmatrix shows us at %f %f %f\n",modelMatrix[12],modelMatrix[13],modelMatrix[14]);

	if (node->geoOrigin) render_node(node->geoOrigin);

	if (GeoVerbose) printf ("GeoLocating to %f %f %f\n",
			(double)node->__geoCoords.c[0]-GeoOrig[0], 
			(double)node->__geoCoords.c[1]-GeoOrig[1],
			(double)node->__geoCoords.c[2]-GeoOrig[2]);

	glTranslated ((double)node->__geoCoords.c[0]-GeoOrig[0], 
			(double)node->__geoCoords.c[1]-GeoOrig[1],
			(double)node->__geoCoords.c[2]-GeoOrig[2]);
	//fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	//printf ("modelmatrix now shows us at %f %f %f\n\n",modelMatrix[12],modelMatrix[13],modelMatrix[14]);

}
