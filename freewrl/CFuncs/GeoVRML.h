
/* from table 4.9 in the spec - supported spacial reference frames */
#define GEO_GD		256
#define GEO_GC		257
#define GEO_UTM		258

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
