/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

extern int GeoSys;      		// which GeoSystem is parsed from the last GeoOrigin
extern double GeoOrig[3];               // last GeoOrigin parsed in lat/long/elvation format


/* geoSystem field is encoded as:
 *
 * mask : 0xf00000	reference frame, eg, GD, GC, UTM
 * mask : 0x0ff000	UTM zone number  
 * mask : 0x000f00	UTM "S" field
 * mask : 0x0000ff	ellipsoid.
 */


/* from table 4.9 in the spec - supported spacial reference frames */
#define GEO_GD		0x100000
#define GEO_GC		0x200000
#define GEO_UTM		0x300000

/* UTM fields */
#define GEO_UTM_ZONE_BASE	0x01000
#define GEO_UTM_S_FLAG		0x00100

/* from table 4.10 in the spec - supported earth ellipsoids */
#define	GEO_AA		1
#define GEO_AM		2
#define GEO_AN		3
#define GEO_BN		4
#define GEO_BR		5
#define GEO_CC		6
#define GEO_CD		7
#define	GEO_EA		8
#define	GEO_EB		9
#define GEO_EC		10
#define GEO_ED		11
#define GEO_EE		12
#define GEO_EF		13
#define GEO_FA		14
#define GEO_HE		15
#define GEO_HO		16
#define GEO_ID		17
#define GEO_IN		18
#define GEO_KA		19
#define GEO_RF		20
#define GEO_SA		21
#define GEO_WD		22
#define GEO_WE		23

/* from table 4.11 - supported earth geoids */
#define GEO_WGS84	30

/* function prototypes */
void geoSystemCompile (struct Multi_String *geoSystem, int *__geoSystem, char *description);
void render_GeoOrigin (struct VRML_GeoOrigin *node);
void render_GeoLocation (struct VRML_GeoLocation *node);
