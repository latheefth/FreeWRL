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

int tcoordsize;
int tcindexsize;

int nspi = $f_n(spine);			/* number of spine points	*/
int nsec = $f_n(crossSection);		/* no. of points in the 2D curve
					   but note that this is verified
					   and coincident points thrown out */

int nori = $f_n(orientation);		/* no. of given orientators
					   which rotate the calculated SCPs =
					   spine-aligned cross-section planes*/ 
int nsca = $f_n(scale);			/* no. of scale parameters	*/

struct SFColor *spine =$f(spine);	/* vector of spine vertices	*/
struct SFVec2f *curve =$f(crossSection);/* vector of 2D curve points	*/
struct SFRotation *orientation=$f(orientation);/*vector of SCP rotations*/

struct VRML_PolyRep *rep_=this_->_intern;/*internal rep, we want to fill*/
struct VRML_PolyRep tess_polyrep;	/* rep for tessellating the caps*/

/* the next variables will point at members of *rep		*/
int   *cindex;				/* field containing indices into
					   the coord vector. Three together
					   indicate which points form a 
					   triangle			*/
float *coord;				/* contains vertices building the
					   triangles as x y z values	*/

float *tcoord;				/* contains vertices building the
					   textures as x y z values	*/

int	*tcindex;			/* field containing texture indices
					   for the vertex. 		*/
int	tcindex_count = 0;		/* placement of tcindexes	*/

int   *norindex; 			/* indices into *normal		*/
float *normal; 				/* (filled in a different function)*/ 


int ntri = 0;			 	/* no. of triangles to be used
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


int circular = 0;			/* is spine  closed?		*/
int tubular=0;				/* is the 2D curve closed?	*/
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

int	tci_ct;				/* Tex Gen index counter	*/

/* variables for calculating smooth normals */
int 	HAVETOSMOOTH;
struct 	pt *facenormals = 0;
int	*pointfaces = 0;
int	*defaultface = 0;
int 	faces;
int	this_face = 0;			/* always counts up		*/
int	tmp;
int 	tmp_polygon;
float point_normal[3];
int 	calc_normind;
float creaseAngle = $f(creaseAngle);
int	ccw = $f(ccw);
int	end_of_sides;			/* for triangle normal generation,
					   keep track of where the sides end
					   and caps begin		*/

/* variables for begin/endcap S,T mapping for textures			*/
float *beginVals;
float *endVals;
struct SFVec2f *crossSection;

int Extru_Verbose = 0;


if (Extru_Verbose) 
	printf ("VRMLExtrusion.pm start\n");

/***********************************************************************
 *
 * Copy and verify cross section - remove coincident points (yes, virginia,
 * one of the NIST tests has this - the pie-shaped convex one
 *
 ************************************************************************/

/* is there anything to this Extrusion??? */
if (nsec < 1) { 
	rep_->ntri=0;
	return;
} else {
	int tmp1, temp_indx;
	int increment, currentlocn;

	crossSection     = malloc(sizeof(crossSection)*nsec*2);
	if (!(crossSection)) die ("can not malloc memory for Extrusion crossSection");


	currentlocn = 0;
	for (tmp1=0; tmp1<nsec; tmp1++) {
		/* save this crossSection */
		crossSection[currentlocn].c[0] = curve[tmp1].c[0];
		crossSection[currentlocn].c[1] = curve[tmp1].c[1];
	
		/* assume that it is not duplicated */
		increment = 1;	

		for (temp_indx=0; temp_indx<currentlocn; temp_indx++) {
			if ((APPROX(crossSection[currentlocn].c[0],crossSection[temp_indx].c[0])) &&
			    (APPROX(crossSection[currentlocn].c[1],crossSection[temp_indx].c[1]))) {
				/* maybe we have a closed curve, so points SHOULD be the same */
				if ((temp_indx != 0) && (tmp1 != (nsec-1))) {
					//printf ("... breaking; increment = 0\n");
					increment = 0;
					break;
				} else { 
					//printf ("... we are tubular\n");
					tubular = 1;
				}
			}
		}
		/* increment the crossSection index, unless it was duplicated */
		currentlocn += increment;
	}
	
	if (Extru_Verbose) 
		printf ("we had nsec %d coords, but now we have %d\n",nsec,currentlocn);
	nsec = currentlocn;
}


/* now that we have removed possible coincident vertices, we can calc ntris */
ntri = 2 * (nspi-1) * (nsec-1);


if (Extru_Verbose) 
	printf ("so, we have ntri %d nspi %d nsec %d\n",ntri,nspi,nsec);

/* check if the spline is closed					*/

if(spine[0].c[0] == spine[nspi-1].c[0] &&
   spine[0].c[1] == spine[nspi-1].c[1] &&
   spine[0].c[2] == spine[nspi-1].c[2]) 
	circular = 1;

if (Extru_Verbose) printf ("tubular %d circular %d\n",tubular, circular); 
 

/************************************************************************
 * calc number of triangles per cap, if caps are enabled and possible	
 */

if($f(beginCap)||$f(endCap)) {
	if(tubular?nsec<4:nsec<3) {
		die("Only two real vertices in crossSection. Caps not possible!");
	}

	if(Extru_Verbose && circular && tubular) {
		printf("Spine and crossSection-curve are closed - how strange! ;-)\n");
		/* maybe we want to fly in this tunnel? Or it is semi 
		   transparent somehow? It is possible to create
		   nice figures if you rotate the cap planes... */
	}

	if(tubular)	nctri=nsec-2;
	else		nctri=nsec-1;	

	if (Extru_Verbose) printf ("nsec = %d, ntri = %d nctri = %d\n",nsec, ntri,nctri);

		/* check if there are colinear points at the beginning of the curve*/
	sec=0;
	while(sec+2<=nsec-1 && 
		/* to find out if two vectors a and b are colinear, 
		   try a.x*b.y=a.y*b.x					*/

		APPROX(0,    (crossSection[sec+1].c[0]-crossSection[0].c[0])
			    *(crossSection[sec+2].c[1]-crossSection[0].c[1])
			  -  (crossSection[sec+1].c[1]-crossSection[0].c[1])
			    *(crossSection[sec+2].c[0]-crossSection[0].c[0]))	
	     ) ncolinear_at_begin++, sec++;

	/* check if there are colinear points at the end of the curve
		in line with the very first point, because we want to
		draw the triangle to there.				*/
	sec=tubular?(nsec-2):(nsec-1);
	while(sec-2>=0 && 
		APPROX(0,    (crossSection[sec  ].c[0]-crossSection[0].c[0])
			    *(crossSection[sec-1].c[1]-crossSection[0].c[1])
			  -  (crossSection[sec  ].c[1]-crossSection[0].c[1])
			    *(crossSection[sec-1].c[0]-crossSection[0].c[0]))	
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
}

/************************************************************************
 * prepare for filling *rep
 */

rep_->ccw = 1;
 
rep_->ntri = ntri + nctri;	/* Thats the no. of triangles representing
				the whole Extrusion Shape.		*/

/* get some memory							*/
cindex  = rep_->cindex   = malloc(sizeof(*(rep_->cindex))*3*(rep_->ntri));
coord   = rep_->coord    =
		malloc(sizeof(*(rep_->coord))*(nspi*nsec+max_ncoord_add)*3);
normal  = rep_->normal   = malloc(sizeof(*(rep_->normal))*3*(rep_->ntri)*3);
norindex= rep_->norindex = malloc(sizeof(*(rep_->norindex))*3*(rep_->ntri));

/* face normals - one face per quad (ie, 2 triangles) 			*/
/* have to make sure that if nctri is odd, that we increment by one	*/


facenormals = malloc(sizeof(*facenormals)*(rep_->ntri+1)/2);

/* for each triangle vertex, tell me which face(s) it is in		*/
pointfaces = malloc(sizeof(*pointfaces)*POINT_FACES*3*rep_->ntri);

/* for each triangle, it has a defaultface...				*/
defaultface = malloc(sizeof(*defaultface)*rep_->ntri);


/*memory for the SCPs. Only needed in this function. Freed later	*/
SCP     = malloc(sizeof(struct SCP)*nspi);
 
/* in C always check if you got the mem you wanted...  >;->		*/
  if(!(pointfaces && defaultface && facenormals && cindex && coord && normal && norindex && SCP )) {
	die("Not enough memory for Extrusion node triangles... ;(");
} 

if (HAVETODOTEXTURES) { /* texture mapping "stuff" */
	/* so, we now have to worry about textures. */
	/* XXX note - this over-estimates; realloc to be exact */

	tcoordsize = (nctri + (ntri*2))*3;

	if (Extru_Verbose) 
		printf ("tcoordsize is %d\n",tcoordsize);
	tcoord = rep_->tcoord = malloc(sizeof(*(rep_->tcoord))*tcoordsize);

	tcindexsize = rep_->ntri*3;
	if (Extru_Verbose) 
		printf ("tcindexsize %d\n",tcindexsize);
	tcindex  = rep_->tcindex   = malloc(sizeof(*(rep_->tcindex))*tcindexsize);

	/* keep around cross section info for tex coord mapping */
	beginVals = malloc(sizeof(float) * 2 * (nsec+1)*100);
	endVals = malloc(sizeof(float) * 2 * (nsec+1)*100);

	if (!(tcoord && tcindex && beginVals && endVals)) 
		die ("Not enough memory Extrusion Tcoords");
}

/* Normal Generation Code */
initialize_smooth_normals();
HAVETOSMOOTH = smooth_normals && (fabs(creaseAngle>0.0001));
for (tmp = 0; tmp < 3*rep_->ntri; tmp++) {
	pointfaces[tmp*POINT_FACES]=0;
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

	if(Extru_Verbose) printf("spi=%d next_spi=%d\n",spi,next_spi); /**/
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

if (Extru_Verbose) printf (" SCP[0].next = %d, nspi = %d\n",SCP[0].next,nspi);


if(SCP[0].next==nspi) {
	spine_is_one_vertix=1;
	if (Extru_Verbose) printf("All spine vertices are the same!\n");

	/* initialize all y and z values with zero, they will		*/
	/* be treated as colinear case later then			*/
	SCP[0].z.x=0; SCP[0].z.y=0; SCP[0].z.z=0;
	SCP[0].y=SCP[0].z;
	for(spi=1;spi<nspi;spi++) {
		SCP[spi].y=SCP[0].y;
		SCP[spi].z=SCP[0].z;
	}
}else{
	if(Extru_Verbose) {
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

	if (Extru_Verbose)
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
		if (Extru_Verbose) {
		printf ("just calculated z for spi 0\n");
		printf("SCP[0].y=[%f,%f,%f], SCP[1].z=[%f,%f,%f]\n",
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
			if (Extru_Verbose) printf ("just calculated z for spi %d\n",spi);
 		}
	}
 
 	if(circular) {
		if (Extru_Verbose) printf ("we are circular\n");
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
		if (Extru_Verbose) printf ("we are not circular\n");

 		/* calc y for first SCP				*/
		VEC_FROM_CDIFF(spine[SCP[0].next],spine[0],SCP[0].y);

 		/* calc y for the last SCP			*/
		/* in the case of 2, nspi-1 = 1, ...prev = 0	*/
		VEC_FROM_CDIFF(spine[nspi-1],spine[SCP[nspi-1].prev],SCP[nspi-1].y);
 
		/* z for the start SESVs is the same as for the next SCP */
		SCP[0].z=SCP[SCP[0].next].z; 
 		/* z for the last SCP is the same as for the one before the last*/
		SCP[nspi-1].z=SCP[SCP[nspi-1].prev].z; 
	
		if (Extru_Verbose) {	
		printf("SCP[0].y=[%f,%f,%f], SCP[0].z=[%f,%f,%f]\n",
			SCP[0].y.x,SCP[0].y.y,SCP[0].y.z,
			SCP[0].z.x,SCP[0].z.y,SCP[0].z.z);
		printf("SCP[1].y=[%f,%f,%f], SCP[1].z=[%f,%f,%f]\n",
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
			if(Extru_Verbose) printf("Found z-Value!\n");
			for(t=spi-1; t>-1; t--)
				SCP[t].z=SCP[spi].z;
 			pos_of_last_zvalue=spi;	
		}
}
 
if(Extru_Verbose) printf("pos_of_last_zvalue=%d\n",pos_of_last_zvalue);
 
 
/* z axis flipping, if VECPT(SCP[i].z,SCP[i-1].z)<0 			*/
/* we can do it here, because it is not needed in the all-colinear case	*/
for(spi=(circular?2:1);spi<nspi;spi++) {
	if(VECPT(SCP[spi].z,SCP[spi-1].z)<0) {
		VECSCALE(SCP[spi].z,-1);
		if(Extru_Verbose) 
		    printf("Extrusion.GenPloyRep: Flipped axis spi=%d\n",spi);
	}
} /* for */

/* One case is missing: whole spine is colinear				*/
if(pos_of_last_zvalue==-1) {
	if (Extru_Verbose) printf("Extrusion.GenPloyRep:Whole spine is colinear!\n");

	/* this is the default, if we don`t need to rotate		*/
	spy.x=0; spy.y=1; spy.z=0;	
	spz.x=0; spz.y=0; spz.z=1;

	if(!spine_is_one_vertix) {
		/* need to find the rotation from SCP[spi].y to (0 1 0)*/
		/* and rotate (0 0 1) and (0 1 0) to be the new y and z	*/
		/* values for all SCPs					*/
		/* I will choose roation about the x and z axis		*/
		double alpha,gamma;	/* angles for the rotation	*/
		
		/* search a non trivial vector along the spine */
		for(spi=1;spi<nspi;spi++) {
			VEC_FROM_CDIFF(spine[spi],spine[0],spp1);
			if(!APPROX(VECSQ(spp1),0))
 				break;
 		}
 			
		/* normalize the non trivial vector */	
		spylen=1/sqrt(VECSQ(spp1)); VECSCALE(spp1,spylen);
		if(Extru_Verbose)
			printf("Reference vector along spine=[%f,%f,%f]\n",
				spp1.x,spp1.y,spp1.z);


		if(!(APPROX(spp1.x,0))) {
			/* get the angle for the x axis rotation	*/
			/* asin of 1.0000 seems to fail sometimes, so */
			if (spp1.x >= 0.99999) { alpha = asin(0.9999);
			} else if (spp1.x <= -0.99999) { alpha = asin(-0.9999);
			} else alpha=asin((double)spp1.x);
			if(APPROX(cos(alpha),0))
				gamma=0;
			else {
				gamma=acos(spp1.y / cos(alpha) );
				if(fabs(sin(gamma)-(-spp1.z/cos(alpha))
					)>fabs(sin(gamma)))
					gamma=-gamma;
			}

 			if(Extru_Verbose) printf("alpha=%f gamma=%f\n",alpha,gamma);

			spy.y=-(cos(alpha)*(-sin(gamma)));
			spy.z=cos(alpha)*cos(gamma);
			spy.x=sin(alpha);
			spz.y=-(sin(alpha)*sin(gamma));
			spz.z=(-sin(alpha))*cos(gamma);
			spz.x=cos(alpha);
		} 
		if(!(APPROX(spp1.z,0))) {
			/* get the angle for the z axis rotation	*/
			/* asin of 1.0000 seems to fail sometimes, so */
			if (spp1.z >= 0.99999) { alpha = asin(0.9999);
			} else if (spp1.z <= -0.99999) { alpha = asin(-0.9999);
			} else alpha=asin((double)spp1.z);
			if(APPROX(cos(alpha),0))
				gamma=0;
			else {
				gamma=acos(spp1.y / cos(alpha) );
				if(fabs(sin(gamma)-(-spp1.x/cos(alpha))
					)>fabs(sin(gamma)))
					gamma=-gamma;
			}

 			if(Extru_Verbose) printf("alpha=%f gamma=%f\n",alpha,gamma);
			spy.y=-(cos(alpha)*(-sin(gamma)));
			spy.x=cos(alpha)*cos(gamma);
			spy.z=sin(alpha);
			spz.y=-(sin(alpha)*sin(gamma));
			spz.x=(-sin(alpha))*cos(gamma);
			spz.z=cos(alpha);
		} 


	} /* else */
 
	/* apply new y and z values to all SCPs	*/
	for(spi=0;spi<nspi;spi++) {
		SCP[spi].y=spy;
		SCP[spi].z=spz;
	}
 
} /* if all colinear */
 
if(Extru_Verbose) {
	for(spi=0;spi<nspi;spi++) {
		printf("SCP[%d].y=[%f,%f,%f], SCP[%d].z=[%f,%f,%f]\n",
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
 			
		MATRIX_FROM_ROTATION(orientation[ori],m);
		VECMM(m,spx);
		VECMM(m,spy);
		VECMM(m,spz);
	} 
 
	for(sec = 0; sec<nsec; sec++) {
		struct pt point;
		float ptx = crossSection[sec].c[0];
		float ptz = crossSection[sec].c[1];
		if(nsca) {
			int sca = (nsca==nspi ? spi : 0);
			ptx *= $f(scale,sca).c[0];
			ptz *= $f(scale,sca).c[1];
 		}
		point.x = ptx;
		point.y = 0; 
		point.z = ptz;

		//printf ("working on sec %d of %d, spine %d of %d\n", sec, nsec, spi, nspi);


	  /* texture mapping for caps - keep vals around */
	  if (HAVETODOTEXTURES) {
	  	if (spi == 0) { /* begin cap vertices */
			//printf ("begin cap vertecies index %d %d \n", sec*2+0, sec*2+1);

			beginVals[sec*2+0] = ptx;
			beginVals[sec*2+1] = ptz;
	   	} else if (spi == (nspi-1)) {  /* end cap vertices */
			//printf ("end cap vertecies index %d %d size %d\n", sec*2+0, sec*2+1, 2 * (nsec+1));
			endVals[(sec*2)+0]=ptx;
			endVals[(sec*2)+1]=ptz;
	   	} 

	   }
	   //printf ("coord index %x sec %d spi %d nsec %d\n",
	   //		&coord[(sec+spi*nsec)*3+0], sec, spi,nsec);

	   coord[(sec+spi*nsec)*3+0] = 
	    spx.x * point.x + spy.x * point.y + spz.x * point.z
	    + $f(spine,spi).c[0];
	   coord[(sec+spi*nsec)*3+1] = 
	    spx.y * point.x + spy.y * point.y + spz.y * point.z
	    + $f(spine,spi).c[1];
	   coord[(sec+spi*nsec)*3+2] = 
	    spx.z * point.x + spy.z * point.y + spz.z * point.z
	    + $f(spine,spi).c[2];

	} /* for(sec */
} /* for(spi */
ncoord=nsec*nspi;
 
 
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
int Atex, Btex, Ctex, Dtex, Etex, Ftex;	/* Tex Coord points */

struct pt ac,bd,	/* help vectors	*/
	ab,cd;		/* help vectors	for testing intersection */
int E,F;		/* third point to be used for the triangles*/	
double u,r,		/* help variables for testing intersection */
	denominator,	/* ... */
	numerator;	/* ... */

if(Extru_Verbose) {
	printf("Coords: \n");

	for(x=0; x<nsec; x++) {
	 for(z=0; z<nspi; z++) {
	 	int xxx = 3*(x+z*nsec);
	 	printf("coord: %d [%f %f %f] ",(x+z*nsec),
			coord[xxx], coord[xxx+1], coord[xxx+2]);
	 	
	 }
	printf("\n");
	}
	printf("\n");
}
	

/* Now, lay out the spines/sections, and generate triangles */

for(x=0; x<nsec-1; x++) {
  for(z=0; z<nspi-1; z++) {
  A=x+z*nsec;
  B=(x+1)+z*nsec;
  C=(x+1)+(z+1)*nsec; 
  D= x+(z+1)*nsec;

  /* texture mapping coords */
  Atex = A; Btex = B; Ctex = C; Dtex = D;

  /* if we are circular, check to see if this is the first tri, or the last */
  /* the vertexes are identical, but for smooth normal calcs, make the    */
  /* indexes the same, too                                                */
  /* note, we dont touch tex coords here.				  */
  // printf ("x %d z %d nsec %d nspi %d\n",x,z,nsec,nspi);

  if (tubular) {
	//printf ("tubular, x %d nsec %d this_face %d\n",x,nsec,this_face);
	if (x==(nsec-2)) {
		B -=(x+1);
		C -=(x+1);
	}
  }

  if (circular) {
	if (z==(nspi-2)) {
		/* last row in column, assume z=nspi-2, subtract this off */
		C -= (z+1)*nsec; 
		D -= (z+1)*nsec;
	}
  }
 
  /* calculate the distance A-C and see, if it is smaller as B-D	*/
  VEC_FROM_COORDDIFF(coord,C,coord,A,ac);
  VEC_FROM_COORDDIFF(coord,D,coord,B,bd);

  if(sqrt(VECSQ(ac))>sqrt(VECSQ(bd))) {
  	E=B; F=D; Etex=Btex; Ftex=Dtex;
  } else {
  	E=C; F=A; Etex=Ctex; Ftex=Atex;
  }

  /* if concave polygons are expected, we also expect intersecting ones
  	so we are testing, whether A-B and D-C intersect	*/
  if(!$f(convex)) {
    	VEC_FROM_COORDDIFF(coord,B,coord,A,ab);
  	VEC_FROM_COORDDIFF(coord,D,coord,C,cd);
	/* ca=-ac */
	if(Extru_Verbose) {
		printf("ab=[%f,%f,%f],cd=[%f,%f,%f]\n",
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
	if(Extru_Verbose) printf("u=%f, r=%f\n",u,r);
	if(u>=0 && u<=1 && r>=0 && r<=1 
		&& (-ac.x)+u*ab.x==r*cd.x
		&& (-ac.y)+u*ab.y==r*cd.y
		&& (-ac.z)+u*ab.z==r*cd.z ) {
		
		if(Extru_Verbose) printf("Intersection found at P=[%f,%f,%f]!\n",
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

  // printf ("Triangle1 %d %d %d\n",D,A,E); 
  /* first triangle  calculate pointfaces, etc, for this face */
  Elev_Tri(triind*3, this_face, D,A,E, TRUE , rep_, facenormals, pointfaces,ccw);

  if (HAVETODOTEXTURES) {
	tcindex[triind*3] = Dtex;
	tcindex[triind*3+2] = Etex;
	tcindex[triind*3+1] = Atex;
  }

  defaultface[triind] = this_face;
  triind++;

  // printf ("Triangle2 %d %d %d\n",B,C,F);
  /* second triangle - pointfaces, etc,for this face  */
  Elev_Tri(triind*3, this_face, B, C, F, TRUE, rep_, facenormals, pointfaces,ccw);

  if (HAVETODOTEXTURES) {
	tcindex[triind*3] = Btex;
	tcindex[triind*3+1] = Ctex;
	tcindex[triind*3+2] = Ftex;
  }

  if ((HAVETODOTEXTURES) && ((triind*3+2) >= tcindexsize)) 
	printf ("INTERNAL ERROR: Extrusion  - tcindex size too small!\n");
  defaultface[triind] = this_face;
  triind ++; 
  this_face ++;

 }
}

/* do normal calculations for the sides, here */
for (tmp=0; tmp<(triind*3); tmp++) {
	if (HAVETOSMOOTH) {
		normalize_ifs_face (&rep_->normal[tmp*3],
			facenormals, pointfaces, cindex[tmp],
			defaultface[tmp/3], creaseAngle);
	} else {
		rep_->normal[tmp*3+0] = facenormals[defaultface[tmp/3]].x;
		rep_->normal[tmp*3+1] = facenormals[defaultface[tmp/3]].y;
		rep_->normal[tmp*3+2] = facenormals[defaultface[tmp/3]].z;
	}
	rep_->norindex[tmp] = tmp;
}
/* keep track of where the sides end, triangle count-wise, for Normal mapping */
end_of_sides = triind*3;

/* tcindexes are TOTALLY different from sides  - set this in case we are 
   doing textures in the end caps */
tci_ct = nspi*nsec; 

if($f(convex)) {
	int endpoint;

        int Sindex, Tindex = 0;
        GLfloat Ssize, Tsize = 0.0;
	int triind_start; 	/* textures need 2 passes */

	/* if not tubular, we need one more triangle */
	if (tubular) endpoint = nsec-3-ncolinear_at_end;
	else endpoint = nsec-2-ncolinear_at_end;


	//printf ("beginCap, starting at triind %d\n",triind);

	/* this is the simple case with convex polygons	*/
	if($f(beginCap)) {
		triind_start = triind;

		for(x=0+ncolinear_at_begin; x<endpoint; x++) {
  			Elev_Tri(triind*3, this_face, 0, x+2, x+1, TRUE , rep_, facenormals, pointfaces,ccw);
  			defaultface[triind] = this_face;
			if (HAVETODOTEXTURES)
				Extru_tex(triind*3, tci_ct, 0 , +x+2, x+1, rep_,ccw,tcindexsize);
			triind ++;
		}

		if(HAVETODOTEXTURES) {
			Extru_ST_map(triind_start,0+ncolinear_at_begin,endpoint,
				beginVals,nsec,rep_,tcoordsize);
			tci_ct+=endpoint-(0+ncolinear_at_begin);
		}
		triind_start+=endpoint-(0+ncolinear_at_begin);
		this_face++;
	} /* if beginCap */
	
	if($f(endCap)) {
		triind_start = triind;

		for(x=0+ncolinear_at_begin; x<endpoint; x++) {
  			Elev_Tri(triind*3, this_face, 0  +(nspi-1)*nsec,
				x+1+(nspi-1)*nsec,x+2+(nspi-1)*nsec,
				TRUE , rep_, facenormals, pointfaces,ccw);
  			defaultface[triind] = this_face;
			if (HAVETODOTEXTURES) 
				Extru_tex(triind*3, tci_ct, 0+(nspi-1)*nsec, 
					x+1+(nspi-1)*nsec, 
					x+2+(nspi-1)*nsec, rep_,ccw,tcindexsize);
			triind ++;
		}
		this_face++;
		if (HAVETODOTEXTURES) 
			Extru_ST_map(triind_start,0+ncolinear_at_begin,endpoint,
				endVals, nsec, rep_,tcoordsize);
	} /* if endCap */
 	//for (tmp=0;tmp<tcindexsize; tmp++) printf ("index1D %d tcindex %d\n",tmp,tcindex[tmp]);
	
} else 
    if($f(beginCap)||$f(endCap)) { 
	/* polygons might be concave-> do tessellation			*/
	/* XXX - no textures yet - Linux Tesselators give me enough headaches; 
	   lets wait until they are all ok before trying texture mapping */

	/* give us some memory - this array will contain tessd triangle counts */
	int *tess_vs;
	struct SFColor *c1;
	GLdouble tess_v[3]; 
	int endpoint;

	tess_vs=malloc(sizeof(*(tess_vs)) * (nsec - 3 - ncolinear_at_end) * 3);
	if (!(tess_vs)) die ("Extrusion - no memory for tesselated end caps");

	/* if not tubular, we need one more triangle */
	if (tubular) endpoint = nsec-1-ncolinear_at_end;
	else endpoint = nsec-ncolinear_at_end;

		
	if($f(beginCap)) {
		global_IFS_Coord_count = 0;
		gluBeginPolygon(global_tessobj);

		for(x=0+ncolinear_at_begin; x<endpoint; x++) {
                	c1 = (struct SFColor *) &rep_->coord[3*x];
			tess_v[0] = c1->c[0]; tess_v[1] = c1->c[1]; tess_v[2] = c1->c[2];
			tess_vs[x] = x;
			gluTessVertex(global_tessobj,tess_v,&tess_vs[x]);
		}
		gluEndPolygon(global_tessobj);
		verify_global_IFS_Coords(ntri*3);

		for (x=0; x<global_IFS_Coord_count; x+=3) {
  			Elev_Tri(triind*3, this_face, global_IFS_Coords[x], 
				global_IFS_Coords[x+2], global_IFS_Coords[x+1],
				TRUE , rep_, facenormals, pointfaces,ccw);
  			defaultface[triind] = this_face;
			triind ++;
		}
		/* Tesselated faces may have a different normal than calculated previously */
		Extru_check_normal (facenormals,this_face,-1.0,rep_,ccw);

		this_face++;
	}	
	
	if($f(endCap)){	
		global_IFS_Coord_count = 0;
		gluBeginPolygon(global_tessobj);

		for(x=0+ncolinear_at_begin; x<endpoint; x++) {
                	c1 = (struct SFColor *) &rep_->coord[3*(x+(nspi-1)*nsec)];
			tess_v[0] = c1->c[0]; tess_v[1] = c1->c[1]; tess_v[2] = c1->c[2];
			tess_vs[x] = x+(nspi-1)*nsec;
			gluTessVertex(global_tessobj,tess_v,&tess_vs[x]);
		}
		gluEndPolygon(global_tessobj);
		verify_global_IFS_Coords(ntri*3);

		for (x=0; x<global_IFS_Coord_count; x+=3) {
  			Elev_Tri(triind*3, this_face, global_IFS_Coords[x], 
				global_IFS_Coords[x+1], global_IFS_Coords[x+2],
				TRUE , rep_, facenormals, pointfaces,ccw);
  			defaultface[triind] = this_face;
			triind ++;
		}
		/* Tesselated faces may have a different normal than calculated previously */
		Extru_check_normal (facenormals,this_face,1.0,rep_,ccw);

		this_face++;
	}

	/* get rid of mallocd memory  for tess */
	free (tess_vs);

    } /* elseif */
} /* end of block */

/* if we have tesselated, we MAY have fewer triangles than estimated, so... */
rep_->ntri=triind;

//for (tmp=0;tmp<tcindexsize; tmp++) printf ("index2 %d tcindex %d\n",tmp,tcindex[tmp]);
/* do normal calculations for the caps here note - no smoothing */
for (tmp=end_of_sides; tmp<(triind*3); tmp++) {
	rep_->normal[tmp*3+0] = facenormals[defaultface[tmp/3]].x;
	rep_->normal[tmp*3+1] = facenormals[defaultface[tmp/3]].y;
	rep_->normal[tmp*3+2] = facenormals[defaultface[tmp/3]].z;
	rep_->norindex[tmp] = tmp;
}

/* do texture mapping calculations for sides */
if (HAVETODOTEXTURES) {
	/* range check - this should NEVER happen... */
	if (tcoordsize <= ((nsec-1)+(nspi-1)*(nsec-1)*3+2)) {
		printf ("INTERNAL ERROR: Extrusion side tcoord calcs nspi %d nsec %d tcoordsize %d\n",
			nspi,nsec,tcoordsize);
	}
	for(sec=0; sec<nsec; sec++) {
		for(spi=0; spi<nspi; spi++) {
			//printf ("tcoord idx %d %d %d tcoordsize %d ",
			//(sec+spi*nsec)*3,(sec+spi*nsec)*3+1,(sec+spi*nsec)*3+2,tcoordsize);
			//printf ("side texts sec %d spi %d\n",sec,spi);
			tcoord[(sec+spi*nsec)*3+0] = (float) sec/(nsec-1);
			tcoord[(sec+spi*nsec)*3+1] = 0;
			tcoord[(sec+spi*nsec)*3+2] = (float) spi/(nspi-1);
			//printf (" %f %f\n",tcoord[(sec+spi*nsec)*3+0],tcoord[(sec+spi*nsec)*3+2]);
		}
	}	
}

if (Extru_Verbose) printf ("done, lets free\n");

/* we no longer need to keep normal-generating memory around */
free (defaultface);
free (pointfaces);
free (facenormals);
free (crossSection);

if (HAVETODOTEXTURES) {
	free (beginVals); 
	free (endVals);
}


if(Extru_Verbose)
	printf("Extrusion.GenPloyRep: triind=%d  ntri=%d nctri=%d "
	"ncolinear_at_begin=%d ncolinear_at_end=%d\n",
	triind,ntri,nctri,ncolinear_at_begin,ncolinear_at_end);
 
if(Extru_Verbose) 
	printf ("end VRMLExtrusion.pm\n");

/*****end of Member Extrusion	*/
';
