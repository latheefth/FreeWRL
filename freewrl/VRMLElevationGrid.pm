# Copyright (C) 1998 Bernhard Reiter and Tuomas J. Lukka
# Copyright (C) 2000 2002 John Stewart, CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# This is the Extrusion -> polyrep code, used by VRMLC.pm to generate
# VRMLFunc.xs &c.

# Extrusion generates 2 triangles per each extrusion step (like elevgrid..)
# The caps, if present, also add some triangles.

'

		int x,z;
		int nx = $f(xDimension);
		float xs = $f(xSpacing);
		int nz = $f(zDimension);
		float zs = $f(zSpacing);
		float *f = $f(height);
		float a[3],b[3];
		int *cindex; 
        	int *tcindex;
		float *coord;
		float *tcoord;
		int *colindex;
		int ntri = (nx && nz ? 2 * (nx-1) * (nz-1) : 0);
		int triind;
		int nf = $f_n(height);
		int cpv = $f(colorPerVertex);
		struct SFColor *colors; int ncolors=0;
		struct VRML_PolyRep *rep_ = this_->_intern;
		struct VRML_Extrusion_Adj *adj; 
		float crease_angle = $f(creaseAngle);
                int npv = $f(normalPerVertex);
                struct SFColor *normals;
                int nnormals = 0;
                int nquadpercol, nquadperrow, nquadcol, nquadrow;

		$fv_null(color, colors, get3, &ncolors);
		$fv_null(normal, normals, get3, &nnormals);
                rep_->ntri = ntri;

		/* printf("Gen elevgrid %d %d %d\\n", ntri, nx, nz); */
		if(nf != nx * nz) {
			die("Elevationgrid: too many / too few: %d %d %d\\n",
				nf, nx, nz);
		}
		if(ncolors) {
			if(!cpv && ncolors < (nx-1) * (nz-1)) {
				die("Elevationgrid: too few colors");
			}
			if(cpv && ncolors < nx*nz) {
				die("Elevationgrid: 2too few colors");
			}
		}
		cindex = rep_->cindex = malloc(sizeof(*(rep_->cindex))*3*(ntri));
		coord = rep_->coord = malloc(sizeof(*(rep_->coord))*nx*nz*3);
		colindex = rep_->colindex = malloc(sizeof(*(rep_->colindex))*3*(ntri));
		if (glIsEnabled(GL_TEXTURE_2D)) {
			/* so, we now have to worry about textures. */
			   
	        	tcindex = rep_->tcindex = malloc(sizeof(*(rep_->tcindex))*3*(ntri));
			tcoord = rep_->tcoord = malloc(sizeof(*(rep_->tcoord))*nx*nz*3);
		}

		/* Flat */
		rep_->normal = malloc(sizeof(*(rep_->normal))*3*ntri*3);
		rep_->norindex = malloc(sizeof(*(rep_->norindex))*3*ntri);	
		adj     = malloc( sizeof(struct VRML_Extrusion_Adj) * nx * nz ); /*AG*/

		/* in C always check if you got the mem you wanted...  >;->		*/
  		if(!(cindex && coord && tcoord && colindex && rep_->normal && rep_->norindex && adj)) {
			die("Not enough memory for ElevationGrid node triangles... ;(");
		} 
 

		/* Prepare the coordinates */

		for(x=0; x<nx; x++) {
		 for(z=0; z<nz; z++) {
		  float h = f[x+z*nx];
		  coord[(x+z*nx)*3+0] = x*xs;
		  coord[(x+z*nx)*3+1] = h;
		  coord[(x+z*nx)*3+2] = z*zs;
		  if (glIsEnabled(GL_TEXTURE_2D)) {
			tcoord[(x+z*nx)*3+0] = (float) x/(nx-1);
		  	tcoord[(x+z*nx)*3+1] = 0;
		  	tcoord[(x+z*nx)*3+2] = (float) z/(nz-1);
		  }
		 }
		}
		/* set the indices to the coordinates		*/
		{
		int A,B,C,D; /* should referr to the four vertices 
				of the polygon	
				(hopefully) counted counter-clockwise, like

				 D----C
				 |    |
				 |    |
				 |    |
				 A----B

				*/
		struct pt ac,bd,/* help vectors	*/
			ab,cd;	/* help vectors	for testing intersection */
		int E,F;	/* third point to be used for the triangles*/
			
		triind = 0;
		for(x=0; x<nx-1; x++) {
		 for(z=0; z<nz-1; z++) {
		  A=x+z*nx;
		  B=(x+1)+z*nx;
		  C=(x+1)+(z+1)*nx;
		  D=x+(z+1)*nx;
		/* calculate the distance A-C and see, 
			if it is smaller as B-D        			*/
		VEC_FROM_COORDDIFF(coord,C,coord,A,ac);
		VEC_FROM_COORDDIFF(coord,D,coord,B,bd);

		if(sqrt(VECSQ(ac))>sqrt(VECSQ(bd))) {
		      E=B; F=D;
		} else {
		      E=C; F=A;
		}
		  
		  /* 1: */
		  cindex[triind*3+0] = A; cindex[triind*3+1] = D; cindex[triind*3+2] = E;
		  if (glIsEnabled(GL_TEXTURE_2D)) {
			tcindex[triind*3+0] =A;
			tcindex[triind*3+1] =D;
			tcindex[triind*3+2] =E;
		  }
		  if(cpv) {
			  colindex[triind*3+0] = A;
			  colindex[triind*3+1] = D;
			  colindex[triind*3+2] = E;
		  } else {
			  colindex[triind*3+0] = x+z*(nx-1);
			  colindex[triind*3+1] = x+z*(nx-1);
			  colindex[triind*3+2] = x+z*(nx-1);
		  }
		rep_->norindex[triind*3+0] = triind;
		rep_->norindex[triind*3+1] = triind;
		rep_->norindex[triind*3+2] = triind;
		  triind ++;
		  /* 2: */
		  cindex[triind*3+0] = C; cindex[triind*3+1] = B; cindex[triind*3+2] = F;
		  if (glIsEnabled(GL_TEXTURE_2D)) {
			tcindex[triind*3+0] = C;
			tcindex[triind*3+1] = B;
			tcindex[triind*3+2] = F;
		  }
		  if(cpv) {
			  colindex[triind*3+0] = C;
			  colindex[triind*3+1] = B;
			  colindex[triind*3+2] = F;
		  } else {
			  colindex[triind*3+0] = x+z*(nx-1);
			  colindex[triind*3+1] = x+z*(nx-1);
			  colindex[triind*3+2] = x+z*(nx-1);
		  }
		  rep_->norindex[triind*3+0] = triind;
		  rep_->norindex[triind*3+1] = triind;
		  rep_->norindex[triind*3+2] = triind;
		  triind ++; 
		 }
		}
		} /* end of block */

		/* smooth normals for the Elevation Grid */
		/* In the part of this system that calculates these normals */
		/* x is north-south,  z is east-west.                       */
		for(x=0; x<nx; x++) {
			for(z=0; z<nz; z++) {
                                if (z == 0){adj[x + z * nx].north_pt = -1;}
                                else {	adj[x + z * nx].north_pt = x + (z-1) * nx; }

                                if (z == nz-1){adj[x + z * nx].south_pt = -1;}
                                else {	adj[x + z * nx].south_pt = x + (z+1) * nx; }

                                if (x == nx-1){adj[x + z * nx].east_pt = -1;}
                                else {	adj[x + z * nx].east_pt = x + (z * nx) + 1;}

                                if (x == 0){adj[x + z * nx].west_pt = -1;}
                                else {	adj[x + z * nx].west_pt = x + (z * nx) - 1;}

                                if ( (	adj[x + z * nx].north_pt == -1) || (adj[x + z * nx].east_pt == -1) ){
                                        adj[x + z * nx].north_east_pt = -1;}
                                else {	adj[x + z * nx].north_east_pt = adj[x + z * nx].north_pt + 1;}

                                if ( (	adj[x + z * nx].north_pt == -1) || (adj[x + z * nx].west_pt == -1) ){
                                        adj[x + z * nx].north_west_pt = -1;}
                                else {	adj[x + z * nx].north_west_pt = adj[x + z * nx].north_pt - 1;}

                                if ( (	adj[x + z * nx].south_pt == -1) || (adj[x + z * nx].east_pt == -1) ){
                                        adj[x + z * nx].south_east_pt = -1;}
                                else {	adj[x + z * nx].south_east_pt = adj[x + z * nx].south_pt + 1;}

                                if ( (	adj[x + z * nx].south_pt == -1) || (adj[x + z * nx].west_pt == -1) ){
                                        adj[x + z * nx].south_west_pt = -1;}
                                else {	adj[x + z * nx].south_west_pt = adj[x + z * nx].south_pt - 1;}
			}
		}
                if (nnormals <= 0){
			if (smooth_normals == -1) {
				/* Needs to be initialized */
	    			glGetIntegerv (GL_SHADE_MODEL, &smooth_normals);
				smooth_normals = smooth_normals == GL_SMOOTH;
			}
                        if (smooth_normals){
                                calc_poly_normals_extrusion(rep_, adj, nx, nz, ntri, 0, crease_angle);

                                /*Flip all the normals to their correct directions. */
                                /*This is ugly in the extreme because it directly   */
                                /*interferes with and relies on the work done in the*/
                                /*previous function call. */
                                for (x=0; x < (ntri*9); x++){
                                        rep_->normal[x]= rep_->normal[x]*(-1);
                                }
                        }
                        else {
                                calc_poly_normals_flat(rep_);
                        }
                }
                else {
                        if(npv){
                                /*normal per vertex*/
                                for (x=0; x < nx*nz; x++){
                                        rep_->normal[x*3+0] = normals[x].c[0];
                                        rep_->normal[x*3+1] = normals[x].c[1];
                                        rep_->normal[x*3+2] = normals[x].c[2];
                                }
                                for (x=0; x<ntri; x++){
                                        rep_->norindex[x*3+0] = rep_->cindex[x*3+0];
                                        rep_->norindex[x*3+1] = rep_->cindex[x*3+1];
                                        rep_->norindex[x*3+2] = rep_->cindex[x*3+2];
                                }
                        }
                        else{
                                /*normal per quad*/
                                for (x=0; x < (nx-1)*(nz-1); x++){
                                        rep_->normal[x*3+0] = normals[x].c[0];
                                        rep_->normal[x*3+1] = normals[x].c[1];
                                        rep_->normal[x*3+2] = normals[x].c[2];
                                }
                                nquadpercol= (nz-1);
                                nquadperrow= (nx-1);
                                for (x=0; x < ntri; x++){
                                        nquadrow = (x/2) % nquadpercol;
                                        nquadcol = (x/2) / nquadpercol;
                                        rep_->norindex[x*3+0] = nquadrow * nquadperrow + nquadcol;
                                        rep_->norindex[x*3+1] = rep_->norindex[x*3+0];
                                        rep_->norindex[x*3+2] = rep_->norindex[x*3+0];
                                        /* printf("%i  triangle:  %i  %i  %i   quad: %i \n",x, rep_->cindex[x*3+0],
                                        rep_->cindex[x*3+1], rep_->cindex[x*3+2], rep_->norindex[x*3+0] ); */
                                }
                        }
                }

                if (adj) free(adj);
	';

