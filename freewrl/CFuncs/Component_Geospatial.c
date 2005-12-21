/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Geospatial Component

*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "Structs.h"
#include "Component_Geospatial.h"

#include "headers.h"
#include "Bindable.h"
#include "Collision.h"

int GeoVerbose = 0;

int GeoSys     = GEO_GD + GEO_WE;	/* which GeoSystem is parsed from the last GeoOrigin */
double GeoOrig[3];			/* last GeoOrigin parsed in lat/long/elvation format */

/* Function Prototypes */
void parse_ellipsoid(int *dest, char *str, char *description);

void make_GeoElevationGrid (struct X3D_GeoElevationGrid * this_) {
}

void render_GeoElevationGrid (struct X3D_GeoElevationGrid * this_) {
                if(!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change)
			regen_polyrep(this_, NULL, this_->color, this_->normal, this_->texCoord);

		if(!this_->solid) {
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_CULL_FACE);
		}
		render_polyrep(this_);

		if(!this_->solid) {
			glPopAttrib();
		}
}

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
	int tmp, tz;
	STRLEN sl;
	STRLEN xx;

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


void prep_GeoOrigin (struct X3D_GeoOrigin *node) {
	STRLEN xx;
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

void prep_GeoLocation (struct X3D_GeoLocation *node) {
	STRLEN xx;
	/* GLdouble modelMatrix[16]; */
	
	if (!render_vp) {
		glPushMatrix();

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
		/*fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
		printf ("modelmatrix shows us at %f %f %f\n",modelMatrix[12],modelMatrix[13],modelMatrix[14]);
		*/

		if (node->geoOrigin) render_node(node->geoOrigin);

		if (GeoVerbose) printf ("GeoLocating to %f %f %f\n",
			(double)node->__geoCoords.c[0]-GeoOrig[0],
			(double)node->__geoCoords.c[1]-GeoOrig[1],
			(double)node->__geoCoords.c[2]-GeoOrig[2]);

		glTranslated ((double)node->__geoCoords.c[0]-GeoOrig[0],
			(double)node->__geoCoords.c[1]-GeoOrig[1],
			(double)node->__geoCoords.c[2]-GeoOrig[2]);
		/*fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
		printf ("modelmatrix now shows us at %f %f %f\n\n",modelMatrix[12],modelMatrix[13],modelMatrix[14]);
		*/
	}

}


void prep_GeoViewpoint (struct X3D_GeoViewpoint *node) {
	double a1;
	char *posnstring;
	STRLEN xx, yy;

	if (!render_vp) return;

	/* printf ("rgvp, node %d ib %d sb %d gepvp\n",node,node->isBound,node->set_bind); */
	/* check the set_bind eventin to see if it is TRUE or FALSE */
	if (node->set_bind < 100) {
		/* up_vector is reset after a bind */
		if (node->set_bind==1) reset_upvector();

		bind_node (node, &viewpoint_tos,&viewpoint_stack[0]);
	}

	if(!node->isBound) return;

	/* stop rendering when we hit A viewpoint or THE viewpoint???
	   shouldnt we check for what_vp????
           maybe only one viewpoint is in the tree at a time? -  ncoder*/

	found_vp = 1; /* We found the viewpoint */

	/* is the position "compiled" yet? */
	if (node->_change != node->_dlchange) {
		/* printf ("have to recompile position...\n"); */
		posnstring = SvPV(node->position,xx);
		if (sscanf (SvPV(node->position,xx),"%f %f %f",&node->__position.c[0],
			&node->__position.c[1], &node->__position.c[2]) != 3) {
			printf ("GeoViewpoint - vp:%s: invalid position string: :%s:\n",
					SvPV(node->description,xx),
					SvPV(node->position,yy));
		}

		geoSystemCompile (&node->geoSystem, &node->__geoSystem,
			SvPV(node->description,xx));

		node->_dlchange = node->_change;
	}

	if (node->geoOrigin) {
		render_node (node->geoOrigin);
	}

	/* perform GeoViewpoint translations */
	glRotated(-node->orientation.r[3]/PI*180.0,node->orientation.r[0],node->orientation.r[1],
		node->orientation.r[2]);
        glTranslated (GeoOrig[0] - node->__position.c[0],
                        GeoOrig[1] - node->__position.c[1],
                        GeoOrig[2] - node->__position.c[2]);

        /* printf ("GeoViewing at %f %f %f\n",
        		GeoOrig[0] - node->__position.c[0],
                        GeoOrig[1] - node->__position.c[1],
                        GeoOrig[2] - node->__position.c[2]); */

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
	calculateFrustumCone();
	/* printf ("render_GeoViewpoint, bound to %d, fieldOfView %f \n",node,node->fieldOfView); */
}

void fin_GeoLocation (struct X3D_GeoLocation *this_) {
	UNUSED (this_);
	if (!render_vp) glPopMatrix ();
}

void child_GeoLOD (struct X3D_GeoLOD *this_) {
	/* do nothing yet */
	UNUSED (this_);
}

void child_GeoLocation (struct X3D_GeoLocation *this_) {
        int nc = (this_->children).n;
        int savedlight = curlight;


        /* any children at all? */
        if (nc==0) return;

        #ifdef CHILDVERBOSE
	printf("RENDER GEOLOCATION START %d (%d)\n",this_, nc);
	#endif

        /* do we have to sort this node? */
        if ((nc > 2 && render_blend)) sortChildren(this_->children);

        /* do we have a DirectionalLight for a child? */
        if(this_->has_light) dirlightChildren(this_->children);

        /* now, just render the non-directionalLight children */
        normalChildren(this_->children);

        if (render_geom && (!render_blend)) {
                /* printf ("geoLocationChild, this is %d, extent %f %f %f\n",
                this_, this_->_extent[0], this_->_extent[1],
                this_->_extent[2]); */
                /*this_->bboxSize.c[0] = this_->_extent[0];
                this_->bboxSize.c[1] = this_->_extent[1];
                this_->bboxSize.c[2] = this_->_extent[2];
                BoundingBox(this_->bboxCenter,this_->bboxSize); */
        }
        
        /* did we have that directionalLight? */
        if((this_->has_light)) glPopAttrib();
        
        #ifdef CHILDVERBOSE
	printf("RENDER GEOLOCATION END %d\n",this_);
	#endif

        
        curlight = savedlight;
}

void changed_GeoLocation ( struct X3D_GeoLocation *this_) { 
                int i;
                int nc = ((this_->children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

                (this_->has_light) = 0;
                for(i=0; i<nc; i++) {
                        p = (struct X3D_Box *)((this_->children).p[i]);
                        if (p->_nodeType == NODE_DirectionalLight) {
                                /*  printf ("group found a light\n");*/
                                (this_->has_light) ++;
                        }
                }
        }


void collide_GeoElevationGrid (struct X3D_GeoElevationGrid *this_) {
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];

	       struct pt t_orig = {0,0,0};
	       static int refnum = 0;

	       struct pt tupv = {0,1,0};
	       struct pt delta = {0,0,0};

	       struct X3D_PolyRep pr;
	       prflags flags = 0;
	       int change = 0;
		STRLEN xx;

		float xSpacing = 0.0;	/* GeoElevationGrid uses strings here */
		float zSpacing = 0.0;	/* GeoElevationGrid uses strings here */
		sscanf (SvPV (this_->xSpacing,xx),"%f",&xSpacing);
		sscanf (SvPV(this_->zSpacing,xx),"%f",&zSpacing);

		/* JAS - first pass, intern is probably zero */
		if (((struct X3D_PolyRep *)this_->_intern) == 0) return;

		/* JAS - no triangles in this text structure */
		if ((((struct X3D_PolyRep *)this_->_intern)->ntri) == 0) return;


	       /*save changed state.*/
	       if(this_->_intern) change = ((struct X3D_PolyRep *)this_->_intern)->_change;
                if(!this_->_intern || this_->_change != ((struct X3D_PolyRep *)this_->_intern)->_change)
                        regen_polyrep(this_, NULL, NULL, NULL, NULL);


 	       if(this_->_intern) ((struct X3D_PolyRep *)this_->_intern)->_change = change;
	       /*restore changes state, invalidates regen_polyrep work done, so it can be done
	         correclty in the RENDER pass */

	       if(!this_->solid) {
		   flags = flags | PR_DOUBLESIDED;
	       }
	       pr = *((struct X3D_PolyRep*)this_->_intern);
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
/*	       if(!fast_ycylinder_sphere_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return; must find data*/


	       delta = elevationgrid_disp(abottom,atop,awidth,astep,pr,this_->xDimension,
				this_->zDimension,xSpacing,zSpacing,
				modelMatrix,flags);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

		#ifdef COLLISIONVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
		   fprintf(stderr,"COLLISION_ELG: ref%d (%f %f %f) (%f %f %f)\n",refnum++,
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );
	       }
		#endif
}


void rendray_GeoElevationGrid (struct X3D_GeoElevationGrid *this_) {
	render_ray_polyrep(this_, NULL);
}
