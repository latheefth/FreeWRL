#ifndef STRUCTSH
#define STRUCTSH



struct pt {
     double x,y,z;

#ifdef CONSTRUCT
#endif

};

/* Internal representation of IndexedFaceSet, Extrusion & ElevationGrid:
 * set of triangles.
 * done so that we get rid of concave polygons etc.
 */
struct VRML_PolyRep { /* Currently a bit wasteful, because copying */
	int _change;
	int ntri; /* number of triangles */
	int alloc_tri; /* number of allocated triangles */
	int *cindex;   /* triples (per triangle) */
	float *coord; /* triples (per point) */
	int *colindex;   /* triples (per triangle) */
	float *color; /* triples or null */
	int *norindex;
	float *normal; /* triples or null */
        int *tcindex; /* triples or null */
        float *tcoord;	/* triples (per triangle) of texture coords */
};


#endif

