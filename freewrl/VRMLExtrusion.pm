# Copyright (C) 1998 Bernhard Reiter and Tuomas J. Lukka
# Copyright (C) 2000 John Stewart, CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# This is the Extrusion -> polyrep code, used by VRMLC.pm to generate
# VRMLFunc.xs &c.

# Extrusion generates 2 triangles per each extrusion step (like elevgrid..)
# The caps, if present, also add some triangles.

'
/*****begin of Member Extrusion	*/
/* This code originates from the file VRMLExtrusion.pm */
int nspi = $f_n(spine);			/* number of spine points	*/
int nsec = $f_n(crossSection);		/* no. of points in the 2D curve*/
int nori = $f_n(orientation);		/* no. of given orientators
					   which rotate the calculated SCPs =
					   spine-aligned cross-section planes*/ 
int nsca = $f_n(scale);			/* no. of scale parameters	*/
struct SFColor *spine =$f(spine);	/* vector of spine vertices	*/
struct SFVec2f *curve =$f(crossSection);/* vector of 2D curve points	*/
struct SFRotation *orientation=$f(orientation);/*vector of SCP rotations*/

struct VRML_PolyRep *rep_=this_->_intern;/*internal rep, we want to fill*/
struct VRML_PolyRep tess_polyrep;	/* rep for tessellating the caps*/

/* the next four variables will point at members of *rep		*/
int   *cindex;				/* field containing indices into
					   the coord vector. Three together
					   indicate which points form a 
					   triangle			*/
float *coord;				/* contains vertices building the
					   triangles as x y z values	*/

int   *norindex; 			/* indices into *normal		*/
float *normal; 				/* (filled in a different function)*/ 


int ntri = 2 * (nspi-1) * (nsec-1);	/* no. of triangles to be used
					   to represent all, but the caps */
int nctri=0;				/* no. of triangles for both caps*/
int nctri_add=0;			/* max no. of add triangles for b.caps*/
int max_ncoord_add=0;			/* max no. of add coords	*/
int ncoord_add=0;			/* no. off added coords		*/
int ncoord=0;				/* no. of used coords		*/

int ncolinear_at_begin=0;		/* no. of triangles which need
					to be skipped, because curve-points
					are in one line at start of curve*/
int ncolinear_at_end=0;			/* no. of triangles which need
					to be skipped, because curve-points
					are in one line at end of curve*/

int spi,sec,triind,pos_of_last_zvalue;	/* help variables 		*/
int next_spi, prev_spi, help;
int t,i;				/* another loop var		*/


int closed = 0;				/* is spine  closed?		*/
int curve_closed=0;			/* is the 2D curve closed?	*/
int spine_is_one_vertix;		/* only one real spine vertix	*/

float spxlen,spylen,spzlen;		/* help vars for scaling	*/

					/* def:struct representing SCPs	*/
struct SCP { 				/* spine-aligned cross-section plane*/
	struct pt y;			/* y axis of SCP		*/
	struct pt z;			/* z axis of SCP		*/
	int prev,next;			/* index in SCP[]
					prev/next different vertix for 
					calculation of this SCP		*/
	   };

struct SCP *SCP;			/* dyn. vector rep. the SCPs	*/

struct pt spm1,spc,spp1,spcp,spy,spz,spoz,spx;	/* help vertix vars	*/

struct VRML_Extrusion_Adj *adj; 	/* Holds the indexes of nodes	*/
					/* that are adjacent for normals*/
					/* calculations			*/

int klm, kmem; 				/* help variables for normals */
float crease_angle;



/*verbose = 1;*/

if (verbose) printf ("VRMLExtrusion.pm start\n");

/* Get the value of GL_SHADE_MODEL to see whether we have to potentially
   calculate smooth normals. Only once per invocation of FreeWRL */

initialize_smooth_normals();


/* do we have a closed curve?						*/
if(curve[0].c[0] == curve[nsec-1].c[0] &&
   curve[0].c[1] == curve[nsec-1].c[1])
	curve_closed=1;

/* check if the spline is closed					*/

if(spine[0].c[0] == spine[nspi-1].c[0] &&
   spine[0].c[1] == spine[nspi-1].c[1] &&
   spine[0].c[2] == spine[nspi-1].c[2]) 
	closed = 1;

if (verbose) printf ("curve_closed %d closed %d\n",curve_closed, closed); 
 

/************************************************************************
 * calc number of triangles per cap, if caps are enabled and possible	
 */

if($f(beginCap)||$f(endCap)) {
	if(curve_closed?nsec<4:nsec<3) {
		die("Only two real vertices in crossSection. Caps not possible!");
	}

	if(verbose && closed && curve_closed) {
		printf("Spine and crossSection-curve are closed - how strange! ;-)\n");
		/* maybe we want to fly in this tunnel? Or it is semi 
		   transparent somehow? It is possible to create
		   nice figures if you rotate the cap planes... */
	}

	if(curve_closed)	nctri=nsec-3;
	else			nctri=nsec-2;	

	if (verbose) printf ("nsec = %d, ntri = %d\n",nsec, ntri);

		/* check if there are colinear points at the beginning of the curve*/
	sec=0;
	while(sec+2<=nsec-1 && 
		/* to find out if two vectors a and b are colinear, 
		   try a.x*b.y=a.y*b.x					*/
		APPROX(0,    (curve[sec+1].c[0]-curve[0].c[0])
			    *(curve[sec+2].c[1]-curve[0].c[1])
			  -  (curve[sec+1].c[1]-curve[0].c[1])
			    *(curve[sec+2].c[0]-curve[0].c[0]))	
	     ) ncolinear_at_begin++, sec++;

	/* check if there are colinear points at the end of the curve
		in line with the very first point, because we want to
		draw the triangle to there.				*/
	sec=curve_closed?(nsec-2):(nsec-1);
	while(sec-2>=0 && 
		APPROX(0,    (curve[sec  ].c[0]-curve[0].c[0])
			    *(curve[sec-1].c[1]-curve[0].c[1])
			  -  (curve[sec  ].c[1]-curve[0].c[1])
			    *(curve[sec-1].c[0]-curve[0].c[0]))	
	     ) ncolinear_at_end++,sec--;

	nctri-= ncolinear_at_begin+ncolinear_at_end;
	if(nctri<1) {
		/* no triangle left :(	*/
		die("All in crossSection points colinear. Caps not possible!");
 	}
 
 
	/* so we have calculated nctri for one cap, but we might have two*/
	nctri= (($f(beginCap))?nctri:0) + (($f(endCap))?nctri:0) ;
}
 
/* if we have non-convex polygons, we might need a few triangles more	*/
/* 	The unused memory will be freed with realloc later		*/
if(!$f(convex)) {

	max_ncoord_add=(nspi-1)*(nsec-1) /* because of intersections	*/
			+nctri;		/* because of cap tesselation	*/
	nctri*=2;	/* we might need more trigs for the caps	*/

	printf ("non-convex polygons, need more triangles, max_ncoord_add %d, nctri %d\n",max_ncoord_add, nctri);
}

/************************************************************************
 * prepare for filling *rep
 */
 
rep_->ntri = ntri + nctri;	/* Thats the no. of triangles representing
				the whole Extrusion Shape.		*/
				
/* Extrusions dont have texture coords, so setting this to 0 always	*/
rep_->tcindex = 0;
	
/* get some memory							*/
cindex  = rep_->cindex   = malloc(sizeof(*(rep_->cindex))*3*(rep_->ntri));
coord   = rep_->coord    =
		malloc(sizeof(*(rep_->coord))*(nspi*nsec+max_ncoord_add)*3);
normal  = rep_->normal   = malloc(sizeof(*(rep_->normal))*3*(rep_->ntri)*3);    /*AG*/
norindex= rep_->norindex = malloc(sizeof(*(rep_->norindex))*3*(rep_->ntri)*3);  /*AG*/ 

 
/*memory for the SCPs. Only needed in this function. Freed later	*/
SCP     = malloc(sizeof(struct SCP)*nspi);


/*memory for the adjacency struct (used for normals)  			*/
adj	= malloc( sizeof(struct VRML_Extrusion_Adj) * nsec * nspi );
kmem = sizeof(struct VRML_Extrusion_Adj) * nsec * nspi;

/*printf("\n  Block of %i allocated \n", kmem );*/

 
/* in C always check if you got the mem you wanted...  >;->		*/
  if(!(cindex && coord && normal && norindex && SCP && adj)) {
	die("Not enough memory for Extrusion node triangles... ;(");
} 
 

/************************************************************************
 * calculate all SCPs 
 */

spine_is_one_vertix=0;

/* fill the prev and next values in the SCP structs first
 *
 *	this is so complicated, because spine vertices can be the same
 *	They should have exactly the same SCP, therefore only one of
 *	an group of sucessive equal spine vertices (now called SESVs)
 *	must be used for calculation.
 *	For calculation the previous and next different spine vertix
 *	must be known. We save that info in the prev and next fields of
 *	the SCP struct. 
 *	Note: We have start and end SESVs which will be treated differently
 *	depending on whether the spine is closed or not
 *
 */
 
for(spi=0; spi<nspi;spi++){
	for(next_spi=spi+1;next_spi<nspi;next_spi++) {
		VEC_FROM_CDIFF(spine[spi],spine[next_spi],spp1);
		if(!APPROX(VECSQ(spp1),0))
			break;
	}
	if(next_spi<nspi) SCP[next_spi].prev=next_spi-1;

	if(verbose) printf("spi=%d next_spi=%d\n",spi,next_spi); /**/
	prev_spi=spi-1;
	SCP[spi].next=next_spi;
	SCP[spi].prev=prev_spi;
	
	while(next_spi>spi+1) { /* fill gaps */
		spi++;
		SCP[spi].next=next_spi;
		SCP[spi].prev=prev_spi;
	}
}
/* now:	start-SEVS .prev fields contain -1				*/
/* 	and end-SEVS .next fields contain nspi				*/


/* calculate the SCPs now...						*/

if (verbose) printf (" SCP[0].next = %d, nspi = %d\n",SCP[0].next,nspi);


if(SCP[0].next==nspi) {
	spine_is_one_vertix=1;
	printf("All spine vertices are the same!\n");

	/* initialize all y and z values with zero, they will		*/
	/* be treated as colinear case later then			*/
	SCP[0].z.x=0; SCP[0].z.y=0; SCP[0].z.z=0;
	SCP[0].y=SCP[0].z;
	for(spi=1;spi<nspi;spi++) {
		SCP[spi].y=SCP[0].y;
		SCP[spi].z=SCP[0].z;
	}
}else{
	if(verbose) {
		for(spi=0;spi<nspi;spi++) {
			printf("SCP[%d].next=%d, SCP[%d].prev=%d\n",
				spi,SCP[spi].next,spi,SCP[spi].prev);
		}
	}
	
	/* find spine vertix different to the first spine vertix	*/
	spi=0; 		
	while(SCP[spi].prev==-1) spi++;

	/* find last spine vertix different to the last 		*/
	t=nspi-1; 
	while(SCP[t].next==nspi) t--;

	if (verbose)
		printf ("now, spi = %d, t = %d\n",spi,t);

	/* for all but the first + last really different spine vertix	*/
	/* add case for then there are only 2 spines, and spi is already */
	/* spi is already greater than t... JAS				*/

	if (spi > t) {
		/* calc y 	*/
		VEC_FROM_CDIFF(spine[1],spine[0],SCP[0].y);
		/* calc z	*/
		VEC_FROM_CDIFF(spine[1],spine[0],spp1);
		VEC_FROM_CDIFF(spine[1],spine[0],spm1);
 		VECCP(spp1,spm1,SCP[1].z);
		if (verbose) {
		printf ("just calculated z for spi 0\n");
		printf("SCP[0].y=[%lf,%lf,%lf], SCP[1].z=[%lf,%lf,%lf]\n",
			SCP[0].y.x,SCP[0].y.y,SCP[0].y.z,
			SCP[1].z.x,SCP[1].z.y,SCP[1].z.z);
		}
	}
	
	else {
		for(; spi<=t; spi++) {
			/* calc y 	*/
			VEC_FROM_CDIFF(spine[SCP[spi].next],spine[SCP[spi].prev],SCP[spi].y);
			/* calc z	*/
			VEC_FROM_CDIFF(spine[SCP[spi].next],spine[spi],spp1);
			VEC_FROM_CDIFF(spine[SCP[spi].prev],spine[spi],spm1);
 			VECCP(spp1,spm1,SCP[spi].z);
			if (verbose) printf ("just calculated z for spi %d\n",spi);
 		}
	}
 
 	if(closed) {
		if (verbose) printf ("we are closed\n");
 		/* calc y for first SCP				*/
		VEC_FROM_CDIFF(spine[SCP[0].next],spine[SCP[nspi-1].prev],SCP[0].y); 
 		/* the last is the same as the first */	
 		SCP[nspi-1].y=SCP[0].y;	
        
		/* calc z */
		VEC_FROM_CDIFF(spine[SCP[0].next],spine[0],spp1);
		VEC_FROM_CDIFF(spine[SCP[nspi-1].prev],spine[0],spm1);
		VECCP(spp1,spm1,SCP[0].z);
		/* the last is the same as the first */	
		SCP[nspi-1].z=SCP[0].z;	
		
 	} else {
		if (verbose) printf ("we are not closed\n");

 		/* calc y for first SCP				*/
		VEC_FROM_CDIFF(spine[SCP[0].next],spine[0],SCP[0].y);

 		/* calc y for the last SCP			*/
		/* in the case of 2, nspi-1 = 1, ...prev = 0	*/
		VEC_FROM_CDIFF(spine[nspi-1],spine[SCP[nspi-1].prev],SCP[nspi-1].y);
 
		/* z for the start SESVs is the same as for the next SCP */
		SCP[0].z=SCP[SCP[0].next].z; 
 		/* z for the last SCP is the same as for the one before the last*/
		SCP[nspi-1].z=SCP[SCP[nspi-1].prev].z; 
	
		if (verbose) {	
		printf("SCP[0].y=[%lf,%lf,%lf], SCP[0].z=[%lf,%lf,%lf]\n",
			SCP[0].y.x,SCP[0].y.y,SCP[0].y.z,
			SCP[0].z.x,SCP[0].z.y,SCP[0].z.z);
		printf("SCP[1].y=[%lf,%lf,%lf], SCP[1].z=[%lf,%lf,%lf]\n",
			SCP[1].y.x,SCP[1].y.y,SCP[1].y.z,
			SCP[1].z.x,SCP[1].z.y,SCP[1].z.z);
		}
	} /* else */
	
	/* fill the other start SESVs SCPs*/
	spi=1; 
	while(SCP[spi].prev==-1) {
		SCP[spi].y=SCP[0].y;
		SCP[spi].z=SCP[0].z;
		spi++;
	}
	/* fill the other end SESVs SCPs*/
	t=nspi-2; 
	while(SCP[t].next==nspi) {
		SCP[t].y=SCP[nspi-1].y;
		SCP[t].z=SCP[nspi-1].z;
		t--;
	}

} /* else */


/* We have to deal with colinear cases, what means z=0			*/
pos_of_last_zvalue=-1;		/* where a zvalue is found */
for(spi=0;spi<nspi;spi++) {
	if(pos_of_last_zvalue>=0) { /* already found one?		*/
		if(APPROX(VECSQ(SCP[spi].z),0)) 
			SCP[spi].z= SCP[pos_of_last_zvalue].z;

		pos_of_last_zvalue=spi;	
	} else 
		if(!APPROX(VECSQ(SCP[spi].z),0)) {
			/* we got the first, fill the previous		*/
			if(verbose) printf("Found z-Value!\n");
			for(t=spi-1; t>-1; t--)
				SCP[t].z=SCP[spi].z;
 			pos_of_last_zvalue=spi;	
		}
}
 
if(verbose) printf("pos_of_last_zvalue=%d\n",pos_of_last_zvalue);
 
 
/* z axis flipping, if VECPT(SCP[i].z,SCP[i-1].z)<0 			*/
/* we can do it here, because it is not needed in the all-colinear case	*/
for(spi=(closed?2:1);spi<nspi;spi++) {
	if(VECPT(SCP[spi].z,SCP[spi-1].z)<0) {
		VECSCALE(SCP[spi].z,-1);
		if(verbose) 
		    printf("Extrusion.GenPloyRep: Flipped axis spi=%d\n",spi);
	}
} /* for */

/* One case is missing: whole spine is colinear				*/
if(pos_of_last_zvalue==-1) {
	if (verbose) printf("Extrusion.GenPloyRep:Whole spine is colinear!\n");

	/* this is the default, if we don`t need to rotate		*/
	spy.x=0; spy.y=1; spy.z=0;	
	spz.x=0; spz.y=0; spz.z=1;

	if(!spine_is_one_vertix) {
		/* need to find the rotation from SCP[spi].y to (0 1 0)*/
		/* and rotate (0 0 1) and (0 1 0) to be the new y and z	*/
		/* values for all SCPs					*/
		/* I will choose roation about the x and z axis		*/
		float alpha,gamma;	/* angles for the rotation	*/
		
		/* search a non trivial vector along the spine */
		for(spi=1;spi<nspi;spi++) {
			VEC_FROM_CDIFF(spine[spi],spine[0],spp1);
			if(!APPROX(VECSQ(spp1),0))
 				break;
 		}
 			
		/* normalize the non trivial vector */	
		spylen=1/sqrt(VECSQ(spp1)); VECSCALE(spp1,spylen);
		if(verbose)
			printf("Reference vector along spine=[%lf,%lf,%lf]\n",
				spp1.x,spp1.y,spp1.z);


		if(!(APPROX(spp1.x,0) && APPROX(spp1.z,0))) {
			/* at least one of x or z is not zero		*/

			/* get the angle for the x axis rotation	*/
			alpha=asin(spp1.z);

			/* get the angle for the z axis rotation	*/
			if(APPROX(cos(alpha),0))
				gamma=0;
			else {
				gamma=acos(spp1.y / cos(alpha) );
				if(fabs(sin(gamma)-(-spp1.x/cos(alpha))
					)>fabs(sin(gamma)))
					gamma=-gamma;
			}

			/* do the rotation (zero values are already worked in)*/
 			if(verbose)
				printf("alpha=%f gamma=%f\n",alpha,gamma);
			spy.x=cos(alpha)*(-sin(gamma));
			spy.y=cos(alpha)*cos(gamma);
			spy.z=sin(alpha);

			spz.x=sin(alpha)*sin(gamma);
			spz.y=(-sin(alpha))*cos(gamma);
			spz.z=cos(alpha);
		} /* if(!spine_is_one_vertix */
	} /* else */
 
	/* apply new y and z values to all SCPs	*/
	for(spi=0;spi<nspi;spi++) {
		SCP[spi].y=spy;
		SCP[spi].z=spz;
	}
 
} /* if all colinear */
 
if(verbose) {
	for(spi=0;spi<nspi;spi++) {
		printf("SCP[%d].y=[%lf,%lf,%lf], SCP[%d].z=[%lf,%lf,%lf]\n",
			spi,SCP[spi].y.x,SCP[spi].y.y,SCP[spi].y.z,
			spi,SCP[spi].z.x,SCP[spi].z.y,SCP[spi].z.z);
	}
}
 

/************************************************************************
 * calculate the coords 
 */

/* test for number of scale and orientation parameters			*/
if(nsca>1 && nsca <nspi)
	printf("Extrusion.GenPolyRep: Warning!\n"
	"\tNumber of scaling parameters do not match the number of spines!\n"
	"\tWill revert to using only the first scale value.\n");

if(nori>1 && nori <nspi)
	printf("Extrusion.GenPolyRep: Warning!\n"
	"\tNumber of orientation parameters "
		"do not match the number of spines!\n"
	"\tWill revert to using only the first orientation value.\n");


for(spi = 0; spi<nspi; spi++) {
	double m[3][3];		/* space for the rotation matrix	*/
	spy=SCP[spi].y; spz=SCP[spi].z;
	VECCP(spy,spz,spx);
	spylen = 1/sqrt(VECSQ(spy)); VECSCALE(spy, spylen);
	spzlen = 1/sqrt(VECSQ(spz)); VECSCALE(spz, spzlen);
	spxlen = 1/sqrt(VECSQ(spx)); VECSCALE(spx, spxlen);

	/* rotate spx spy and spz			*/
	if(nori) {
		int ori = (nori==nspi ? spi : 0);
		
		if(IS_ROTATION_VEC_NOT_NORMAL(orientation[ori]))
			printf("Extrusion.GenPolyRep: Warning!\n"
			  "\tRotationvector #%d not normal!\n"
			  "\tWon`t correct it, because it is bad VRML`97.\n",
			  ori+1); 
 			
		/* first variante:*/ 
		MATRIX_FROM_ROTATION(orientation[ori],m);
		VECMM(m,spx);
		VECMM(m,spy);
		VECMM(m,spz);
		/* */

		/* alternate code (second variant): */ 
		/*
		VECRROTATE(orientation[ori],spx);
		VECRROTATE(orientation[ori],spy);
		VECRROTATE(orientation[ori],spz);
		/* */
	} 
 
	for(sec = 0; sec<nsec; sec++) {
		struct pt point;
		float ptx = curve[sec].c[0];
		float ptz = curve[sec].c[1];
		if(nsca) {
			int sca = (nsca==nspi ? spi : 0);
			ptx *= $f(scale,sca).c[0];
			ptz *= $f(scale,sca).c[1];
 		}
		point.x = ptx;
		point.y = 0; 
		point.z = ptz;

	   coord[(sec+spi*nsec)*3+0] = 
	    spx.x * point.x + spy.x * point.y + spz.x * point.z
	    + $f(spine,spi).c[0];
	   coord[(sec+spi*nsec)*3+1] = 
	    spx.y * point.x + spy.y * point.y + spz.y * point.z
	    + $f(spine,spi).c[1];
	   coord[(sec+spi*nsec)*3+2] = 
	    spx.z * point.x + spy.z * point.y + spz.z * point.z
	    + $f(spine,spi).c[2];

		/*
		printf("Point    x: %lf    y: %lf    z: %lf  \n",
			coord[(sec+spi*nsec)*3+0], 
			coord[(sec+spi*nsec)*3+1], 
			coord[(sec+spi*nsec)*3+2]  );
		*/

		/* Specify the relationship this point has with	      */
		/* those around him.  This data is collected here     */
		/* for calculating normals to obtain a smooth surface.*/
		/*                                                           */
		/* Imagine that you are looking at the side of an extrusion. */
		/* Each point is surrounded by four quadrilaterals.          */
		/* These are the names given to the neighbouring points.     */
		/*                                                           */
		/*         north_west_pt          north_pt       north_east_pt   */
		/*                      *-----------*-----------*                */
		/*                      |           |           |                */
		/*                      |  4th quad | 1st quad  |                */
		/*                      |           |           |                */
		/*                      |           |           |                */
		/*                      |           |           |                */
		/*        west_pt       *-----------*-----------* east_pt  */
		/*                      |           |           |                */
		/*                      |  3rd quad | 2nd quad  |                */
		/*                      |           |           |                */
		/*                      |           |           |                */
		/*                      |           |           |                */
		/*                      *-----------*-----------*                */
		/*         south_west_pt          south_pt       south_east_pt   */
		/*                                                               */
		/* I cannot recommend trying to modify this code -> I took *days**/
		/* to get right.                                                 */

		if(spi == 0){
			if(closed){
				adj[spi * nsec + sec].north_pt = (nspi-2) * nsec + sec;
			}
			else{
				adj[spi * nsec + sec].north_pt = -1;
			}
		}
		else {
			adj[spi * nsec + sec].north_pt = (spi-1) * nsec + sec; 
		}


		/*  set south_pt  */
		if(spi == nspi-1){
			if(closed){
				adj[spi * nsec + sec].south_pt = nsec + sec;
			}
			else{
				adj[spi * nsec + sec].south_pt = -1;
			}
		}
		else{
				adj[spi * nsec + sec].south_pt = (spi+1) * nsec + sec ;
		}


		/*  set west_pt  */
		if(sec == 0){
			if(curve_closed){
				adj[spi * nsec + sec].west_pt = spi * (nsec) + nsec -2;
			}
			else{
				adj[spi * nsec + sec].west_pt =  -1;
			}
		}
		else{
			adj[spi * nsec + sec].west_pt = spi * nsec + sec -1;
		}


                /*  set east_pt  */
                if(sec == nsec-1){
                        if(curve_closed){
                                adj[spi * nsec + sec].east_pt = spi * (nsec) + 1 ;
                        }
                        else{
                                adj[spi * nsec + sec].east_pt =  -1;
                        }
                }
                else{
                        adj[spi * nsec + sec].east_pt = spi * nsec + sec + 1;
                }


		/* More data collection, this is again needed for smooth normals. */
		
		if( (adj[spi * nsec + sec].north_pt == -1) 
		|| (adj[spi * nsec + sec].east_pt == -1 ) ) {
			adj[spi * nsec + sec].north_east_pt = -1;
		}
		else if (curve_closed && (sec == nsec-1) && !closed){
			adj[spi * nsec + sec].north_east_pt = (spi-1) * nsec + 1;
		}
		else if (curve_closed && closed && (sec == nsec-1) && !(spi == 0) ){
			adj[spi * nsec + sec].north_east_pt = (spi-1) * nsec + 1;
		}
		else if (curve_closed && closed && (sec == nsec-1) && (spi == 0) ){
			adj[spi * nsec + sec].north_east_pt = (nspi-2) * nsec + 1; 
		}
		else{
			adj[spi * nsec + sec].north_east_pt = adj[spi * nsec + sec].north_pt + 1;
		}

		
		if( (adj[spi * nsec + sec].east_pt == -1) 
		|| (adj[spi * nsec + sec].south_pt == -1 ) ) {
			adj[spi * nsec + sec].south_east_pt = -1;
		}
		else if (curve_closed && (sec == nsec-1) && !closed){
			adj[spi * nsec + sec].south_east_pt = (spi+1) * nsec + 1;
		}
		else if (curve_closed && closed && (sec == nsec-1) && !(spi == nspi-1) ){
			adj[spi * nsec + sec].south_east_pt = (spi+1) * nsec + 1; 
		}
		else if (curve_closed && closed && (sec == nsec-1) && (spi == nspi-1) ){
			adj[spi * nsec + sec].south_east_pt = 1*nsec + 1; 
		}
		else {
			adj[spi * nsec + sec].south_east_pt = adj[spi * nsec + sec].south_pt + 1;
		}

		
		if( (adj[spi * nsec + sec].south_pt == -1) 
		|| (adj[spi * nsec + sec].west_pt == -1 ) ) {
			adj[spi * nsec + sec].south_west_pt = -1;
		}
		else if (curve_closed && (sec == 0) && !closed){
			adj[spi * nsec + sec].south_west_pt = (spi+1) * nsec + nsec - 2;
		}
		else if (curve_closed && closed && (sec == 0) && !(spi == nspi-1) ){
			adj[spi * nsec + sec].south_west_pt = (spi+1) * nsec + nsec - 2; 
		}
		else if (curve_closed && closed && (sec == 0) && (spi == nspi-1) ){
			adj[spi * nsec + sec].south_west_pt = 1*nsec +nsec-2; 
		}
		else {
			adj[spi * nsec + sec].south_west_pt = adj[spi * nsec + sec].south_pt - 1;
		}


		if( (adj[spi * nsec + sec].west_pt == -1) 
		|| (adj[spi * nsec + sec].north_pt == -1 ) ) {
			adj[spi * nsec + sec].north_west_pt = -1;
		}
		else if (curve_closed && (sec == 0) && !closed){
			adj[spi * nsec + sec].north_west_pt = (spi-1) * nsec + nsec - 2;
		}
		else if (curve_closed && closed && (sec == 0) && !(spi == 0) ){
			adj[spi * nsec + sec].north_west_pt = (spi-1) * nsec + nsec - 2; 
		}
		else if (curve_closed && closed && (sec == 0) && (spi == 0) ){
			adj[spi * nsec + sec].north_west_pt = (nspi-2) * nsec + nsec -2; 
		}
		else {
			adj[spi * nsec + sec].north_west_pt = adj[spi * nsec + sec].north_pt - 1;
		}


	} /* for(sec */

} /* for(spi */
ncoord=nsec*nspi;

/**DEBUG CODE*****************************************************
for(klm=0; klm < nspi*nsec; klm++ ){
	printf("%i   south_pt: %i\n", klm, adj[klm].south_pt);
	printf("%i   north_pt: %i\n", klm, adj[klm].north_pt);
	printf("%i   east_pt: %i\n", klm, adj[klm].east_pt);
	printf("%i   west_pt: %i\n", klm, adj[klm].west_pt);
	printf("%i   north_east_pt: %i\n", klm, adj[klm].north_east_pt);
	printf("%i   south_east_pt: %i\n", klm, adj[klm].south_east_pt);
	printf("%i   south_west_pt: %i\n", klm, adj[klm].south_west_pt);
	printf("%i   north_west_pt: %i\n", klm, adj[klm].north_west_pt);
	printf("-----------------------------------------------\n");
}
printf("nsec: %i       nspi: %i \n", nsec, nspi);
*/



/* freeing SCP coordinates. not needed anymore.				*/
if(SCP) free(SCP);
 
/************************************************************************
 * setting the values of *cindex to the right coords
 */
 
triind = 0;
{
int x,z; 
int A,B,C,D; /* should referr to the four vertices of the polygon	
		(hopefully) counted counter-clockwise, like
		 
		 D----C
		 |    |
		 |    |
		 |    |
		 A----B
		 
		*/
struct pt ac,bd,	/* help vectors	*/
	ab,cd;		/* help vectors	for testing intersection */
int E,F;		/* third point to be used for the triangles*/	
double u,r,		/* help variables for testing intersection */
	denominator,	/* ... */
	numerator;	/* ... */

if(verbose) {
	printf("Coords: \n");

	for(x=0; x<nsec; x++) {
	 for(z=0; z<nspi; z++) {
	 	int xxx = 3*(x+z*nsec);
	 	printf("[%f %f %f] ",
			coord[xxx], coord[xxx+1], coord[xxx+2]);
	 	
	 }
	printf("\n");
	}
	printf("\n");
}
	
for(x=0; x<nsec-1; x++) {
 for(z=0; z<nspi-1; z++) {
  A=x+z*nsec;
  B=(x+1)+z*nsec;
  C=(x+1)+(z+1)*nsec; 
  D= x+(z+1)*nsec;
  
  /* calculate the distance A-C and see, if it is smaller as B-D	*/
  VEC_FROM_COORDDIFF(coord,C,coord,A,ac);
  VEC_FROM_COORDDIFF(coord,D,coord,B,bd);

  if(sqrt(VECSQ(ac))>sqrt(VECSQ(bd))) {
  	E=B; F=D;
  } else {
  	E=C; F=A;
  }


  /* if concave polygons are expected, we also expect intersecting ones
  	so we are testing, whether A-B and D-C intersect	*/
  if(!$f(convex)) {
    	VEC_FROM_COORDDIFF(coord,B,coord,A,ab);
  	VEC_FROM_COORDDIFF(coord,D,coord,C,cd);
	/* ca=-ac */
	if(verbose) {
		printf("ab=[%lf,%lf,%lf],cd=[%lf,%lf,%lf]\n",
			ab.x,ab.y,ab.z,cd.x,cd.y,cd.z);
		printf("Orig: %d %d  [%f %f %f] [%f %f %f] (%d, %d, %d) \n",
				D, C,
				coord[D*3], coord[D*3+1], coord[D*3+2],
				coord[C*3], coord[C*3+1], coord[C*3+2],
				ncoord, nsec, nspi
		);
	}
	denominator= ab.y*cd.x-ab.x*cd.y;
	numerator  = (-ac.x)*cd.y-(-ac.y)*cd.x;
	
	r=u=-1;
	if(!APPROX(denominator,0)) {
		u=numerator/denominator;
		r=((-ac.x)*ab.y-(-ac.y)*ab.x)/denominator;
	} else {
		/* lines still may be coincident*/
		if(APPROX(numerator,0)) {
			/* we have to calculate u and r using the z coord*/
			denominator=ab.z*cd.x-ab.x*cd.z;
			numerator  = (-ac.x)*cd.z-(-ac.z)*cd.x;
			if(!APPROX(denominator,0)) {
			u=numerator/denominator;
			r=((-ac.x)*ab.y-(-ac.y)*ab.x)/denominator;
			} 
		}
	} /* else */
	if(verbose) printf("u=%lf, r=%lf\n",u,r);
	if(u>=0 && u<=1 && r>=0 && r<=1 
		&& (-ac.x)+u*ab.x==r*cd.x
		&& (-ac.y)+u*ab.y==r*cd.y
		&& (-ac.z)+u*ab.z==r*cd.z ) {
		
		if(verbose) printf("Intersection found at P=[%lf,%lf,%lf]!\n",
			coord[A*3]+u*ab.x,
			coord[A*3+1]+u*ab.y,
			coord[A*3+2]+u*ab.y
			);
		coord[(ncoord)*3  ]=coord[A*3  ]+u*ab.x;
		coord[(ncoord)*3+1]=coord[A*3+1]+u*ab.y;
		coord[(ncoord)*3+2]=coord[A*3+2]+u*ab.z;
		E=ncoord;
		F=ncoord;
		ncoord_add++;
		ncoord++;
	}

  } 

  
  /* first triangle */
  cindex[triind*3+0] = D; cindex[triind*3+1] = A; cindex[triind*3+2] = E;

  /* smooth normals and flat normals use completely different indexing  */
  /* schemes */
  if(smooth_normals){
  	norindex[triind*3+0] = D;	/**AG**/ 
  	norindex[triind*3+1] = A; 
  	norindex[triind*3+2] = E; 
  	/*printf("triangle : %i  D: %i  A: %i  E: %i \n ", triind, D, A, E);*/
  }
  else {
	norindex[triind*3+0] = triind;
	norindex[triind*3+1] = triind;
	norindex[triind*3+2] = triind;
  }
  triind ++;

  /* second triangle*/
  cindex[triind*3+0] = B; cindex[triind*3+1] = C; cindex[triind*3+2] = F;

  if(smooth_normals){
  	norindex[triind*3+0] = B; 	/**AG**/
  	norindex[triind*3+1] = C; 
  	norindex[triind*3+2] = F; 
	/*printf("triangle : %i  B: %i  C: %i  F: %i \n ", triind, B, C, F);*/
  }
  else {
  	norindex[triind*3+0] = triind;
  	norindex[triind*3+1] = triind;
  	norindex[triind*3+2] = triind;
  }
  triind ++; 
 }
}

/* for the caps */

if(verbose) {
	if($f(beginCap)) 
		printf("Extrusion.GenPloyRep:We have a beginCap!\n"); 
	if($f(endCap)) 
		printf("Extrusion.GenPloyRep:We have a endCap!\n"); 
}
	
	
if($f(convex)) {
	/* this is the simple case with convex polygons	*/
	if($f(beginCap)) {
		for(x=0+ncolinear_at_begin; x<nsec-3-ncolinear_at_end; x++) {
			cindex[triind*3+0] = 0;
			cindex[triind*3+1] = x+2;
			cindex[triind*3+2] = x+1;

			if(!smooth_normals){
				norindex[triind*3+0] = triind;	/**AG**/
				norindex[triind*3+1] = triind;
				norindex[triind*3+2] = triind;
			}
			triind ++;
		}
		if(!curve_closed) {	/* non closed need one triangle more	*/
			cindex[triind*3+0] = 0;
			cindex[triind*3+1] = x+2;
			cindex[triind*3+2] = x+1;

			if(!smooth_normals){
				norindex[triind*3+0] = triind;	/**AG**/
				norindex[triind*3+1] = triind;
				norindex[triind*3+2] = triind;
			}
			triind ++;
 		}
	} /* if beginCap */
	
	if($f(endCap)) {
		for(x=0+ncolinear_at_begin; x<nsec-3-ncolinear_at_end; x++) {
			/* endCap not showing, because it is facing wrong way.	*/
			/* try changing how triangles are drawn			*/
			cindex[triind*3+2] = 0  +(nspi-1)*nsec;
			cindex[triind*3+1] = x+2+(nspi-1)*nsec;
			cindex[triind*3+0] = x+1+(nspi-1)*nsec;

			if(!smooth_normals){
				norindex[triind*3+0] = triind;	/**AG**/
				norindex[triind*3+1] = triind;
				norindex[triind*3+2] = triind;
			}
			triind ++;
		}
		if(!curve_closed) {	/* non closed needs one triangle more	*/
			/* lets flip 0 and 2 around; endCaps were being drawn inside-out */
			cindex[triind*3+0] = 0  +(nspi-1)*nsec;
			cindex[triind*3+1] = x+2+(nspi-1)*nsec;
			cindex[triind*3+2] = x+1+(nspi-1)*nsec;

			if(!smooth_normals){
				norindex[triind*3+0] = triind;	/**AG**/
				norindex[triind*3+1] = triind;
				norindex[triind*3+2] = triind;
			}
			triind ++;
 		}
	} /* if endCap */
	
} else 
    if($f(beginCap)||$f(endCap)) { 
	/* polygons might be concave-> do tessellation			*/

	GLdouble tess_v[3];		/*param.to gluTessVertex()*/
	GLdouble *tess_vs;		/* pointer to space needed */
	struct pt help_pt;		/* help vertix		*/
	int ncoord_new=0;		/* # of coords added	*/
	

    	if(verbose)printf("Extrusion.GenPolyRep: Trying to tessellate caps.\n");
	
	nctri=0;
	tess_polyrep.ntri= nsec*2 ;	
	tess_polyrep.alloc_tri= nsec*2 ;	
				/* max number of resulting tris -
				   2*nsec + caps */
		
					/* get memory	*/
	tess_polyrep.cindex=malloc(
			sizeof(*(tess_polyrep.cindex))*3*(tess_polyrep.ntri));
	tess_polyrep.coord=malloc(
			sizeof(*(tess_polyrep.coord))*9*(tess_polyrep.ntri));
	tess_vs=malloc(sizeof(*(tess_vs))*9*(tess_polyrep.ntri));
	if(!(tess_polyrep.cindex&&tess_polyrep.coord&&tess_vs))
		die("Got no memory!\n");
		
		
	if($f(beginCap)){	
		tess_polyrep.ntri=0;	/* first triangle index to be filled*/
		global_tess_polyrep=&tess_polyrep;
		gluBeginPolygon(global_tessobj);
		gluNextContour(global_tessobj,GLU_UNKNOWN);
		help=curve_closed?nsec-1:nsec;
		for(sec=0;sec<help;sec++) {
			tess_v[0]=tess_vs[sec*3]  =coord[sec*3];
			tess_v[1]=tess_vs[sec*3+1]=coord[sec*3+1];
			tess_v[2]=tess_vs[sec*3+2]=coord[sec*3+2];
			/* the third argument is the pointer, we get back*/
			gluTessVertex(global_tessobj,tess_v,&tess_vs[sec*3]);
		}
		gluEndPolygon(global_tessobj);
		
		
if(verbose) {
		for(t=0;t<tess_polyrep.ntri;t++) {
		    for(i=0;i<3;i++) {
	        	printf("coord[%dff]=%lf,%lf,%lf\n",
			   tess_polyrep.cindex[t*3+i]*3,
			   tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3],
			   tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+1],
			   tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+2]);
		    }
		}
}
		
		ncoord_new=0;
		for(t=0;t<tess_polyrep.ntri;t++) {
		    for(i=0;i<3;i++) {
			/* see if the needed coords are already there	*/
			cindex[triind*3+i]=-1;
			for(sec=0;sec<help;sec++) {
				VEC_FROM_COORDDIFF(tess_polyrep.coord,tess_polyrep.cindex[t*3+i],coord,sec,help_pt);
				if(APPROX(VECSQ(help_pt),0)) {
		 			cindex[triind*3+i]=sec;
					break;  
		 		}
			}
			if(cindex[triind*3+i]==-1)
			    for(sec=ncoord-ncoord_new;sec<ncoord;sec++) {
				VEC_FROM_COORDDIFF(tess_polyrep.coord,tess_polyrep.cindex[t*3+i],coord,sec,help_pt);
				if(APPROX(VECSQ(help_pt),0)) {
		 			cindex[triind*3+i]=sec;
					break;
				}  
			    }

			if(cindex[triind*3+i]==-1) {
			  /* we need to add a new coord   */
			  coord[ncoord*3]  =
			     tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3];
			  coord[ncoord*3+1]=
			     tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+1];
			  coord[ncoord*3+2]=
			     tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+2];
			  cindex[triind*3+i]=ncoord;
			  ncoord_add++;
			  ncoord_new++;
			  ncoord++;
			}	
			norindex[triind*3+i] = triind;
		    }
		    triind++; nctri++;
		}
	} /* if beginCap */
	
	
	
	if($f(endCap)){	
		tess_polyrep.ntri=0;	/* first triangle index to be filled*/
		global_tess_polyrep=&tess_polyrep;
		gluBeginPolygon(global_tessobj);
		gluNextContour(global_tessobj,GLU_UNKNOWN);
		help=curve_closed?nsec-1:nsec;
		for(sec=0;sec<help;sec++) {
		      tess_v[0]=tess_vs[sec*3]  =coord[((nspi-1)*nsec+sec)*3];
		      tess_v[1]=tess_vs[sec*3+1]=coord[((nspi-1)*nsec+sec)*3+1];
		      tess_v[2]=tess_vs[sec*3+2]=coord[((nspi-1)*nsec+sec)*3+2];
		      /* the third argument is the pointer, we get back*/
		      gluTessVertex(global_tessobj,tess_v,&tess_vs[sec*3]);
		}
		gluEndPolygon(global_tessobj);
		
		
		if(verbose) {
		for(t=0;t<tess_polyrep.ntri;t++) {
		    for(i=0;i<3;i++) {
	        	printf("coord[%dff]=%lf,%lf,%lf\n",
			   tess_polyrep.cindex[t*3+i]*3,
			   tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3],
			   tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+1],
			   tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+2]);
		    }
		}
		}

		help=(curve_closed?nsec-1:nsec)+(nspi-1)*nsec;
		ncoord_new=0;
		for(t=0;t<tess_polyrep.ntri;t++) {
		    for(i=0;i<3;i++) {
			/* see if the needed coords are already there	*/
			cindex[triind*3+i]=-1;
			for(sec=(nspi-1)*nsec;sec<help;sec++) {
				VEC_FROM_COORDDIFF(tess_polyrep.coord,tess_polyrep.cindex[t*3+i],coord,sec,help_pt);

				/*printf("help_pt=[%lf,%lf,%lf]\n",help_pt.x,help_pt.y,help_pt.z); */
				if(APPROX(VECSQ(help_pt),0)) {

					/*printf("vertex found at %d\n",sec); */
		 			cindex[triind*3+i]=sec;
					break;  
		 		}
			}
			if(cindex[triind*3+i]==-1)
			    for(sec=ncoord-ncoord_new;sec<ncoord;sec++) {
				VEC_FROM_COORDDIFF(tess_polyrep.coord,tess_polyrep.cindex[t*3+i],coord,sec,help_pt);
				if(APPROX(VECSQ(help_pt),0)) {

					if (verbose) 
					printf("vertex found at %d\n",sec);

 
		 			cindex[triind*3+i]=sec;
					break;
				}  
			    }

			if(cindex[triind*3+i]==-1) {
			  /* we need to add a new coord   */
			  coord[ncoord*3]  =
			     tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3];
			  coord[ncoord*3+1]=
			     tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+1];
			  coord[ncoord*3+2]=
			     tess_polyrep.coord[tess_polyrep.cindex[t*3+i]*3+2];
			  cindex[triind*3+i]=ncoord;
			  ncoord_add++;
			  ncoord_new++;
			  ncoord++;
			}	
			norindex[triind*3+i] = triind;
		    }
		    triind++; nctri++;
		}
	} /* if endCap */	
	
	if(tess_polyrep.coord) free(tess_polyrep.coord);
	if(tess_polyrep.cindex) free(tess_polyrep.cindex);
	if(tess_vs) free(tess_vs);
	
    } /* elseif */
 

} /* end of block */

/* free memory we haven`t used	*/
if(!$f(convex)) {
	if(ncoord_add<max_ncoord_add)
		realloc(coord,sizeof(*(rep_->coord))*(ncoord)*3);
	if(triind<rep_->ntri) {
		rep_->ntri=triind;
		realloc(cindex,sizeof(*(rep_->cindex))*3*(rep_->ntri));
		realloc(normal,sizeof(*(rep_->normal))*3*(rep_->ntri)*3);   /*AG*/
		realloc(norindex,sizeof(*(rep_->norindex))*3*(rep_->ntri)*3); /*AG*/
	}
}


if(verbose)
	printf("Extrusion.GenPloyRep: triind=%d  ntri=%d nctri=%d "
	"ncolinear_at_begin=%d ncolinear_at_end=%d\n",
	triind,ntri,nctri,ncolinear_at_begin,ncolinear_at_end);

	crease_angle = $f(creaseAngle);

if (smooth_normals){
	calc_poly_normals_extrusion(rep_, adj, nspi, nsec, ntri, nctri, crease_angle); 
}
else {
	calc_poly_normals_flat(rep_);
}


if(adj) free(adj);  /**AG**/


if(verbose) printf ("end VRMLExtrusion.pm\n");

/*verbose = 0;*/

/*****end of Member Extrusion	*/
';
