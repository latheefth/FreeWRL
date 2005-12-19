# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Name:        VRMLRend.c
# Description:
#              Fills Hash Variables with "C" Code. They are used by VRMLC.pm
#              to write the C functions-source to render different nodes.
#
#              Certain Abbreviation are used, some are substituted in the
#              writing process in get_rendfunc() [VRMLC.pm].
#              Others are "C-#defines".
#              e.g. for #define glTexCoord2f(a,b) glTexCoord2f(a,b) see gen() [VRMLC.pm]
#
#              Hashes filled in this file:
#                      %RendC, %PrepC, %FinC, %ChildC, %LightC
#
# $Log$
# Revision 1.191  2005/12/19 21:25:08  crc_canada
# HAnim start
#
# Revision 1.190  2005/12/16 18:07:11  crc_canada
# rearrange perl generation
#
# Revision 1.189  2005/12/16 13:49:23  crc_canada
# updating generation functions.
#
# Revision 1.188  2005/12/15 20:42:01  crc_canada
# CoordinateInterpolator2D PositionInterpolator2D
#
# Revision 1.187  2005/12/15 19:57:58  crc_canada
# Geometry2D nodes complete.
#
# Revision 1.186  2005/12/14 13:51:32  crc_canada
# More Geometry2D nodes.
#
# Revision 1.185  2005/12/13 17:00:29  crc_canada
# Arc2D work.
#.....



#######################################################################
#######################################################################
#######################################################################
#
# Rend --
#  actually render the node
#
#

# Rend = real rendering - rend_geom is true; this is for things that
#	actually affect triangles/lines on the screen.
#
# All of these will have a render_xxx name associated with them.

%RendC = map {($_=>1)} qw/
	NavigationInfo
	Fog
	Background
	TextureBackground
	Box 
	Cylinder 
	Cone 
	Sphere 
	IndexedFaceSet 
	Extrusion 
	ElevationGrid 
	Arc2D 
	ArcClose2D 
	Circle2D 
	Disk2D 
	Polyline2D 
	Polypoint2D 
	Rectangle2D 
	TriangleSet2D 
	IndexedTriangleFanSet 
	IndexedTriangleSet 
	IndexedTriangleStripSet 
	TriangleFanSet 
	TriangleStripSet 
	TriangleSet 
	LineSet 
	IndexedLineSet 
	PointSet 
	GeoElevationGrid 
	LoadSensor 
	TextureCoordinateGenerator 
	TextureCoordinate 
	Text 
	LineProperties 
	FillProperties 
	Material 
	PixelTexture 
	ImageTexture 
	MultiTexture 
	MovieTexture 
	Sound 
	AudioClip 
	DirectionalLight 
	HAnimHumanoid
	HAnimJoint
/;

#######################################################################
#######################################################################
#######################################################################
#
# Prep --
#  Prepare for rendering a node - e.g. for transforms, do the transform
#  but not the children.
#
#

%PrepC = (
# this creates the Struct values required to allow backend to fill the C values out
HAnimJoint=> 'prep_HAnimJoint ((struct X3D_HAnimJoint *) this_);',
HAnimSite=> 'prep_HAnimSite ((struct X3D_HAnimSite *) this_);',


GeoOrigin => 'render_GeoOrigin ((struct X3D_GeoOrigin *) this_);',

Viewpoint => '
	/* Viewpoint is in the PrepC side of things, as it is rendered before other nodes */
	if (!render_vp) return;
	render_Viewpoint ((struct X3D_Viewpoint*) this_);',

GeoViewpoint => '
	/* Viewpoint is in the PrepC side of things, as it is rendered before other nodes */
	if (!render_vp) return;
	render_GeoViewpoint ((struct X3D_GeoViewpoint*) this_);',

GeoLocation => '
	if (!render_vp) {
		glPushMatrix();
		render_GeoLocation ((struct X3D_GeoLocation*) this_);
	}',

Transform => '

	GLfloat my_rotation;
	GLfloat my_scaleO=0;
	int	recalculate_dist;

        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	 /* we recalculate distance on last pass, or close to it, and only
	 once per event-loop tick. we can do it on the last pass - the
	 render_sensitive pass, but when mouse is clicked (eg, moving in
	 examine mode, sensitive node code is not rendered. So, we choose
	 the second-last pass. ;-) */
	recalculate_dist = render_light;

	/* printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	if(!render_vp) {
                /* glPushMatrix();*/
		fwXformPush(this_);

		/* might we have had a change to a previously ignored value? */
		if (this_->_change != this_->_dlchange) {
			/* printf ("re-rendering for %d\n",this_);*/
			this_->__do_center = verify_translate ((GLfloat *)this_->center.c);
			this_->__do_trans = verify_translate ((GLfloat *)this_->translation.c);
			this_->__do_scale = verify_scale ((GLfloat *)this_->scale.c);
			this_->__do_rotation = verify_rotate ((GLfloat *)this_->rotation.r);
			this_->__do_scaleO = verify_rotate ((GLfloat *)this_->scaleOrientation.r);
			this_->_dlchange = this_->_change;
		}



		/* TRANSLATION */
		if (this_->__do_trans)
			glTranslatef(this_->translation.c[0],this_->translation.c[1],this_->translation.c[2]);

		/* CENTER */
		if (this_->__do_center)
			glTranslatef(this_->center.c[0],this_->center.c[1],this_->center.c[2]);

		/* ROTATION */
		if (this_->__do_rotation) {
			my_rotation = this_->rotation.r[3]/3.1415926536*180;
			glRotatef(my_rotation,
				this_->rotation.r[0],this_->rotation.r[1],this_->rotation.r[2]);
		}

		/* SCALEORIENTATION */
		if (this_->__do_scaleO) {
			my_scaleO = this_->scaleOrientation.r[3]/3.1415926536*180;
			glRotatef(my_scaleO, this_->scaleOrientation.r[0],
				this_->scaleOrientation.r[1],this_->scaleOrientation.r[2]);
		}


		/* SCALE */
		if (this_->__do_scale)
			glScalef(this_->scale.c[0],this_->scale.c[1],this_->scale.c[2]);

		/* REVERSE SCALE ORIENTATION */
		if (this_->__do_scaleO)
			glRotatef(-my_scaleO, this_->scaleOrientation.r[0],
				this_->scaleOrientation.r[1],this_->scaleOrientation.r[2]);

		/* REVERSE CENTER */
		if (this_->__do_center)
			glTranslatef(-this_->center.c[0],-this_->center.c[1],-this_->center.c[2]);

		/* did either we or the Viewpoint move since last time? */
		if (recalculate_dist) {
			/* printf ("calling PointInView for %d\n",this_);*/
			this_->PIV = PointInView(this_);
			/* printf ("ppv %d\n",this_->PIV);*/

	       }
        }
',
Billboard => '
	struct pt vpos, ax, cp, cp2, arcp;
	static const struct pt orig = {0.0, 0.0, 0.0};
	static const struct pt zvec = {0.0, 0.0, 1.0};
	struct orient viewer_orient;
	GLdouble mod[16];
	GLdouble proj[16];

	int align;
	double len, len2, angle;
	int sign;
	ax.x = $f(axisOfRotation,0);
	ax.y = $f(axisOfRotation,1);
	ax.z = $f(axisOfRotation,2);
	align = (APPROX(VECSQ(ax),0));

	quaternion_to_vrmlrot(&(Viewer.Quat),
		&(viewer_orient.x), &(viewer_orient.y),
		&(viewer_orient.z), &(viewer_orient.a));

	glPushMatrix();

	fwGetDoublev(GL_MODELVIEW_MATRIX, mod);
	fwGetDoublev(GL_PROJECTION_MATRIX, proj);
	gluUnProject(orig.x, orig.y, orig.z, mod, proj,
		viewport, &vpos.x, &vpos.y, &vpos.z);

	len = VECSQ(vpos);
	if (APPROX(len, 0)) { return; }
	VECSCALE(vpos, 1/sqrt(len));

	if (align) {
		ax.x = viewer_orient.x;
		ax.y = viewer_orient.y;
		ax.z = viewer_orient.z;
	}

	VECCP(ax, zvec, arcp);
	len = VECSQ(arcp);
	if (APPROX(len, 0)) { return; }

	len = VECSQ(ax);
	if (APPROX(len, 0)) { return; }
	VECSCALE(ax, 1/sqrt(len));

	VECCP(vpos, ax, cp); /* cp is now 90deg to both vector and axis */
	len = sqrt(VECSQ(cp));
	if (APPROX(len, 0)) {
		glRotatef(-viewer_orient.a/3.1415926536*180, ax.x, ax.y, ax.z);
		return;
	}
	VECSCALE(cp, 1/len);

	/* Now, find out angle between this and z axis */
	VECCP(cp, zvec, cp2);

	len2 = VECPT(cp, zvec); /* cos(angle) */
	len = sqrt(VECSQ(cp2)); /* this is abs(sin(angle)) */

	/* Now we need to find the sign first */
	if (VECPT(cp, arcp) > 0) { sign = -1; } else { sign = 1; }
	angle = atan2(len2, sign*len);

	glRotatef(angle/3.1415926536*180, ax.x, ax.y, ax.z);
',

);

#######################################################################
#######################################################################
#######################################################################
#
# Fin --
#  Finish the rendering i.e. restore matrices and whatever to the
#  original state.
#
#

# Finish rendering
%FinC = (
GeoLocation => (join '','
	UNUSED(this_);
	if (!render_vp) glPopMatrix();
	'),

Transform => (join '','

	if(!render_vp) {
            /* glPopMatrix();*/
	    fwXformPop(this_);
	} else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
		glTranslatef(',(join ',',map {getf(Transform,center,$_)} 0..2),'
		);
		glRotatef(',getf(Transform,scaleOrientation,3),'/3.1415926536*180,',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glScalef(',(join ',',map {"1.0/(".getf(Transform,scale,$_).")"} 0..2),'
		);
		glRotatef(-(',getf(Transform,scaleOrientation,3),'/3.1415926536*180),',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glRotatef(-(',getf(Transform,rotation,3),')/3.1415926536*180,',
			(join ',',map {getf(Transform,rotation,$_)} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,center,$_).")"} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,translation,$_).")"}
			0..2),'
		);
            }
        }

'),


HAnimSite => (join '','
	if(!render_vp) {
            /* glPopMatrix();*/
	    fwXformPop(this_);
	} else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
		glTranslatef(',(join ',',map {getf(Transform,center,$_)} 0..2),'
		);
		glRotatef(',getf(Transform,scaleOrientation,3),'/3.1415926536*180,',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glScalef(',(join ',',map {"1.0/(".getf(Transform,scale,$_).")"} 0..2),'
		);
		glRotatef(-(',getf(Transform,scaleOrientation,3),'/3.1415926536*180),',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glRotatef(-(',getf(Transform,rotation,3),')/3.1415926536*180,',
			(join ',',map {getf(Transform,rotation,$_)} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,center,$_).")"} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,translation,$_).")"}
			0..2),'
		);
            }
        }

'),

HAnimJoint => (join '','
	if(!render_vp) {
            /* glPopMatrix();*/
	    fwXformPop(this_);
	} else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
		glTranslatef(',(join ',',map {getf(Transform,center,$_)} 0..2),'
		);
		glRotatef(',getf(Transform,scaleOrientation,3),'/3.1415926536*180,',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glScalef(',(join ',',map {"1.0/(".getf(Transform,scale,$_).")"} 0..2),'
		);
		glRotatef(-(',getf(Transform,scaleOrientation,3),'/3.1415926536*180),',
			(join ',',map {getf(Transform,scaleOrientation,$_)} 0..2),'
		);
		glRotatef(-(',getf(Transform,rotation,3),')/3.1415926536*180,',
			(join ',',map {getf(Transform,rotation,$_)} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,center,$_).")"} 0..2),'
		);
		glTranslatef(',(join ',',map {"-(".getf(Transform,translation,$_).")"}
			0..2),'
		);
            }
        }

'),


Billboard => (join '','
	UNUSED (this_);
	glPopMatrix();
'),
);

#######################################################################
#######################################################################
#######################################################################
#
# Child --
#  Render the actual children of the node.
#
#

# Render children (real child nodes, not e.g. appearance/geometry)
%ChildC = (
	HAnimHumanoid => 'child_HAnimHumanoid((struct X3D_HAnimHumanoid *) this_); ',
	HAnimJoint => 'child_HAnimJoint((struct X3D_HAnimJoint *) this_); ',
	HAnimSegment => 'child_HAnimSegment((struct X3D_HAnimSegment *) this_); ',
	HAnimSite => 'child_HAnimSite((struct X3D_HAnimSite *) this_); ',

	Group => 'groupingChild(this_); ',
	StaticGroup => 'staticGroupingChild(this_); ',
	Billboard => 'billboardChild(this_); ',
	Transform => 'transformChild(this_); ',
	Anchor => 'anchorChild(this_); ',
	GeoLocation => 'geolocationChild(this_); ',
	Inline => 'inlineChild(this_); ',
	InlineLoadControl => 'inlinelodChild (this_); ',
	Switch => '
		/* exceedingly simple - render only one child */
		int wc = $f(whichChoice);
		if(wc >= 0 && wc < $f_n(choice)) {
			void *p = $f(choice,wc);
			render_node(p);
		}
	',
	GeoLOD => 'UNUSED (this_);',
	LOD => 'lodChild(this_);',
	Collision => 'collisionChild(this_);',

	Appearance => '
		last_texture_depth = 0;
		last_transparency = 1.0;

	/* printf ("in Appearance, this %d, nodeType %d\n",this_, this_->_nodeType);
	 printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	 render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

		if($f(material)) {
			render_node($f(material));
		} else {
			/* no material, so just colour the following shape */
                       	/* Spec says to disable lighting and set coloUr to 1,1,1 */
                       	glDisable (GL_LIGHTING);
			glColor3f(1.0,1.0,1.0);
			lightingOn = FALSE;
		}

		if ($f(fillProperties)) {
			render_node($f(fillProperties));
		}

		/* set line widths - if we have line a lineProperties node */
		if ($f(lineProperties)) {
			render_node($f(lineProperties));
		}

		if($f(texture)) {
			/* we have to do a glPush, then restore, later */
			have_texture=TRUE;
			glPushAttrib(GL_ENABLE_BIT); 

			/* is there a TextureTransform? if no texture, fugutaboutit */
			this_textureTransform = this_->textureTransform;

			/* now, render the texture */
			render_node($f(texture));
		}

	',
	Shape => '
		int trans;
		int should_rend;
		GLdouble modelMatrix[16];
		int count;

		if(!(this_->geometry)) { return; }

		/* do we need to do some distance calculations? */
		if (((!render_vp) && render_light)) {
			fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
			this_->_dist = modelMatrix[14];
			/* printf ("getDist - recalculating distance, it is %f for %d\n",*/
			/* 	this_->_dist,this_);*/
		}

		if((render_collision) || (render_sensitive)) {
			/* only need to forward the call to the child */
			render_node((this_->geometry));
			return;
		}

		/* reset textureTransform pointer */
		this_textureTransform = 0;
		global_lineProperties=FALSE;
		global_fillProperties=FALSE;



		/* JAS - if not collision, and render_geom is not set, no need to go further */
		/* printf ("render_Shape vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
		/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

		/* a texture and a transparency flag... */
		texture_count = 0; /* will be >=1 if textures found */
		trans = have_transparency;
		have_texture = FALSE;

                /* assume that lighting is enabled. Absence of Material or Appearance
                   node will turn lighting off; in this case, at the end of Shape, we
                   have to turn lighting back on again. */
                lightingOn = TRUE;

		/* is there an associated appearance node? */
       	        if($f(appearance)) {
                        render_node($f(appearance));
       	        } else {
                        /* no material, so just colour the following shape */
                       	/* Spec says to disable lighting and set coloUr to 1,1,1 */
                       	glDisable (GL_LIGHTING);
       	                glColor3f(1.0,1.0,1.0);
			lightingOn = FALSE;

			/* tell the rendering passes that this is just "normal" */
			last_texture_depth = 0;
			last_transparency = 1.0;
                }


		/* lets look at texture depth, and if it has alpha, call
		it a transparent node */
		if (last_texture_depth >3) {
			have_transparency++;
			if ((this_->_renderFlags & VF_Blend) != VF_Blend)
				update_renderFlag(this_,VF_Blend);
		}

		/* printf ("Shape, last_trans %d this trans %d last_texture_depth %d\n",*/
		/* 	have_transparency, trans, last_texture_depth);*/

		should_rend = FALSE;
		/* now, are we rendering blended nodes? */
		if (render_blend) {
			if (have_transparency!=trans) {
					should_rend = TRUE;
			}

		/* no, maybe we are rendering straight nodes? */
		} else {
			if (have_transparency == trans) {
					should_rend = TRUE;
			}
		}

		/* if (should_rend) {printf ("RENDERING THIS ONE\n");*/
		/* } else { printf ("NOT RENDERING THIS ONE\n");}*/

		/* should we render this node on this pass? */
		if (should_rend) {
			render_node((this_->geometry));
		}

               /* did the lack of an Appearance or Material node turn lighting off? */
                if (!lightingOn) {
                        glEnable (GL_LIGHTING);
                }

		/* any line properties to reset? */
		if (global_lineProperties) {
			glDisable (GL_POLYGON_STIPPLE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		if (global_fillProperties) {
			glDisable (GL_LINE_STIPPLE);
			glLineWidth(1.0);
		}

		if (have_texture) glPopAttrib(); 
	',
);

#######################################################################
#######################################################################
#######################################################################
#
# Light --
#  Render a light. XXX This needs work to be like the spec :(
#
#

# NO startlist -- nextlight() may change :(
%LightC = (
	PointLight => '
		if($f(on)) {
			int light = nextlight();
			if(light >= 0) {
				float vec[4];
				glEnable(light);
				vec[0] = $f(direction,0);
				vec[1] = $f(direction,1);
				vec[2] = $f(direction,2);
				vec[3] = 1;
				glLightfv(light, GL_SPOT_DIRECTION, vec);
				vec[0] = $f(location,0);
				vec[1] = $f(location,1);
				vec[2] = $f(location,2);
				vec[3] = 1;
				glLightfv(light, GL_POSITION, vec);

				glLightf(light, GL_CONSTANT_ATTENUATION,
					$f(attenuation,0));
				glLightf(light, GL_LINEAR_ATTENUATION,
					$f(attenuation,1));
				glLightf(light, GL_QUADRATIC_ATTENUATION,
					$f(attenuation,2));


				vec[0] = $f(color,0) * $f(intensity);
				vec[1] = $f(color,1) * $f(intensity);
				vec[2] = $f(color,2) * $f(intensity);
				vec[3] = 1;
				glLightfv(light, GL_DIFFUSE, vec);
				glLightfv(light, GL_SPECULAR, vec);

				/* Aubrey Jaffer */
				vec[0] = $f(color,0) * $f(ambientIntensity);
				vec[1] = $f(color,1) * $f(ambientIntensity);
				vec[2] = $f(color,2) * $f(ambientIntensity);
				glLightfv(light, GL_AMBIENT, vec);

				/* XXX */
				glLightf(light, GL_SPOT_CUTOFF, 180);
			}
		}
	',
	SpotLight => '
		if($f(on)) {
			int light = nextlight();
			if(light >= 0) {
				float vec[4];
				/* glEnable(light); */
				lightState(light-GL_LIGHT0,TRUE);

				vec[0] = $f(direction,0);
				vec[1] = $f(direction,1);
				vec[2] = $f(direction,2);
				vec[3] = 1;
				glLightfv(light, GL_SPOT_DIRECTION, vec);
				vec[0] = $f(location,0);
				vec[1] = $f(location,1);
				vec[2] = $f(location,2);
				vec[3] = 1;
				glLightfv(light, GL_POSITION, vec);

				glLightf(light, GL_CONSTANT_ATTENUATION,
					$f(attenuation,0));
				glLightf(light, GL_LINEAR_ATTENUATION,
					$f(attenuation,1));
				glLightf(light, GL_QUADRATIC_ATTENUATION,
					$f(attenuation,2));


				vec[0] = $f(color,0) * $f(intensity);
				vec[1] = $f(color,1) * $f(intensity);
				vec[2] = $f(color,2) * $f(intensity);
				vec[3] = 1;
				glLightfv(light, GL_DIFFUSE, vec);
				glLightfv(light, GL_SPECULAR, vec);

				/* Aubrey Jaffer */
				vec[0] = $f(color,0) * $f(ambientIntensity);
				vec[1] = $f(color,1) * $f(ambientIntensity);
				vec[2] = $f(color,2) * $f(ambientIntensity);

				glLightfv(light, GL_AMBIENT, vec);

				/* XXX */
				glLightf(light, GL_SPOT_EXPONENT,
					0.5/($f(beamWidth)+0.1));
				glLightf(light, GL_SPOT_CUTOFF,
					$f(cutOffAngle)/3.1415926536*180);
			}
		}
	',
);

#######################################################################
#
# ExtraMem - extra members for the structures to hold
# 	cached info

%ExtraMem = (
	Group => 'int has_light; ',
);

$ExtraMem{Transform} = $ExtraMem{Group};
$ExtraMem{Billboard} = $ExtraMem{Group};
$ExtraMem{Anchor} = $ExtraMem{Group};
$ExtraMem{Collision} = $ExtraMem{Group};
$ExtraMem{GeoLocation} = $ExtraMem{Group};
$ExtraMem{Inline} = $ExtraMem{Group};
$ExtraMem{InlineLoadControl} = $ExtraMem{Group};
$ExtraMem{StaticGroup} = $ExtraMem{Group};
$ExtraMem{HAnimSite} = $ExtraMem{Group};
$ExtraMem{HAnimHumanoid} = $ExtraMem{Group};


#######################################################################
#
# ChangedC - when the fields change, the following code is run before
# rendering for caching the data.
#
#

%ChangedC = (
	Group => '
		int i;
		int nc = $f_n(children);
		struct X3D_Box *p;
		struct X3D_Virt *v;

		$i(has_light) = 0;
		for(i=0; i<nc; i++) {
			p = (struct X3D_Box *)$f(children,i);
			if (p->_nodeType == NODE_DirectionalLight) {
				/*  printf ("group found a light\n");*/
				$i(has_light) ++;
			}
		}
        ',
        Inline => '
                int i;
                int nc = $f_n(__children);
                struct X3D_Box *p;
                struct X3D_Virt *v;

                $i(has_light) = 0;
                for(i=0; i<nc; i++) {
                        p = (struct X3D_Box *)$f(__children,i);
                        if (p->_nodeType == NODE_DirectionalLight) {
                                /*  printf ("group found a light\n");*/
                                $i(has_light) ++;
                        }
                }
        '
);


$ChangedC{Transform} = $ChangedC{Group};
$ChangedC{StaticGroup} = $ChangedC{Group};
$ChangedC{Billboard} = $ChangedC{Group};
$ChangedC{Anchor} = $ChangedC{Group};
$ChangedC{Collision} = $ChangedC{Group};
$ChangedC{GeoLocation} = $ChangedC{Group};
$ChangedC{InlineLoadControl} = $ChangedC{Group};
$ChangedC{HAnimSite} = $ChangedC{Group};


#######################################################################
#
# ProximityC = following code is run to let proximity sensors send their
# events. This is done in the rendering pass, because the position of
# of the object relative to the viewer is available via the
# modelview transformation matrix.
#

%ProximityC = (
ProximitySensor => q~
	/* Viewer pos = t_r2 */
	double cx,cy,cz;
	double len;
	struct pt dr1r2;
	struct pt dr2r3;
	struct pt nor1,nor2;
	struct pt ins;
	static const struct pt yvec = {0,0.05,0};
	static const struct pt zvec = {0,0,-0.05};
	static const struct pt zpvec = {0,0,0.05};
	static const struct pt orig = {0,0,0};
	struct pt t_zvec, t_yvec, t_orig;
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];

	if(!$f(enabled)) return;

	/* printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	/* transforms viewers coordinate space into sensors coordinate space.
	 * this gives the orientation of the viewer relative to the sensor.
	 */
	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(orig.x,orig.y,orig.z,modelMatrix,projMatrix,viewport,
		&t_orig.x,&t_orig.y,&t_orig.z);
	gluUnProject(zvec.x,zvec.y,zvec.z,modelMatrix,projMatrix,viewport,
		&t_zvec.x,&t_zvec.y,&t_zvec.z);
	gluUnProject(yvec.x,yvec.y,yvec.z,modelMatrix,projMatrix,viewport,
		&t_yvec.x,&t_yvec.y,&t_yvec.z);

	cx = t_orig.x - $f(center,0);
	cy = t_orig.y - $f(center,1);
	cz = t_orig.z - $f(center,2);

	if($f(size,0) == 0 || $f(size,1) == 0 || $f(size,2) == 0) return;

	if(fabs(cx) > $f(size,0)/2 ||
	   fabs(cy) > $f(size,1)/2 ||
	   fabs(cz) > $f(size,2)/2) return;

	/* Ok, we now have to compute... */
	$f(__hit) = 1;

	/* Position */
	$f(__t1,0) = t_orig.x;
	$f(__t1,1) = t_orig.y;
	$f(__t1,2) = t_orig.z;

	VECDIFF(t_zvec,t_orig,dr1r2);  /* Z axis */
	VECDIFF(t_yvec,t_orig,dr2r3);  /* Y axis */

	len = sqrt(VECSQ(dr1r2)); VECSCALE(dr1r2,1/len);
	len = sqrt(VECSQ(dr2r3)); VECSCALE(dr2r3,1/len);

	#ifdef RENDERVERBOSE
	printf("PROX_INT: (%f %f %f) (%f %f %f) (%f %f %f)\n (%f %f %f) (%f %f %f)\n",
		t_orig.x, t_orig.y, t_orig.z,
		t_zvec.x, t_zvec.y, t_zvec.z,
		t_yvec.x, t_yvec.y, t_yvec.z,
		dr1r2.x, dr1r2.y, dr1r2.z,
		dr2r3.x, dr2r3.y, dr2r3.z
		);
	#endif

	if(fabs(VECPT(dr1r2, dr2r3)) > 0.001) {
		printf ("Sorry, can't handle unevenly scaled ProximitySensors yet :("
		  "dp: %f v: (%f %f %f) (%f %f %f)\n", VECPT(dr1r2, dr2r3),
		  	dr1r2.x,dr1r2.y,dr1r2.z,
		  	dr2r3.x,dr2r3.y,dr2r3.z
			);
		return;
	}


	if(APPROX(dr1r2.z,1.0)) {
		/* rotation */
		$f(__t2,0) = 0;
		$f(__t2,1) = 0;
		$f(__t2,2) = 1;
		$f(__t2,3) = atan2(-dr2r3.x,dr2r3.y);
	} else if(APPROX(dr2r3.y,1.0)) {
		/* rotation */
		$f(__t2,0) = 0;
		$f(__t2,1) = 1;
		$f(__t2,2) = 0;
		$f(__t2,3) = atan2(dr1r2.x,dr1r2.z);
	} else {
		/* Get the normal vectors of the possible rotation planes */
		nor1 = dr1r2;
		nor1.z -= 1.0;
		nor2 = dr2r3;
		nor2.y -= 1.0;

		/* Now, the intersection of the planes, obviously cp */
		VECCP(nor1,nor2,ins);

		len = sqrt(VECSQ(ins)); VECSCALE(ins,1/len);

		/* the angle */
		VECCP(dr1r2,ins, nor1);
		VECCP(zpvec, ins, nor2);
		len = sqrt(VECSQ(nor1)); VECSCALE(nor1,1/len);
		len = sqrt(VECSQ(nor2)); VECSCALE(nor2,1/len);
		VECCP(nor1,nor2,ins);

		$f(__t2,3) = -atan2(sqrt(VECSQ(ins)), VECPT(nor1,nor2));

		/* rotation  - should normalize sometime... */
		$f(__t2,0) = ins.x;
		$f(__t2,1) = ins.y;
		$f(__t2,2) = ins.z;
	}
	#ifdef RENDERVERBOSE
	printf("NORS: (%f %f %f) (%f %f %f) (%f %f %f)\n",
		nor1.x, nor1.y, nor1.z,
		nor2.x, nor2.y, nor2.z,
		ins.x, ins.y, ins.z
	);
	#endif
~,


);

#######################################################################
#
# CollisionC = following code is run to do collision detection
#
# In collision nodes:
#    if enabled:
#       if no proxy:
#           passes rendering to its children
#       else (proxy)
#           passes rendering to its proxy
#    else
#       does nothing.
#
# In normal nodes:
#    uses gl modelview matrix to determine distance from viewer and
# angle from viewer. ...
#
#
#	       /* the shape of the avatar is a cylinder */
#	       /*                                           */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           |--|                            */
#	       /*           | width                         */
#	       /*        ---|---       -                    */
#	       /*        |     |       |                    */
#	       /*    ----|() ()| - --- | ---- y=0           */
#	       /*        |  \  | |     |                    */
#	       /*     -  | \ / | |head | height             */
#	       /*    step|     | |     |                    */
#	       /*     -  |--|--| -     -                    */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           x,z=0                           */


%CollisionC = (

Disk2D => q~
		/* yeah, yeah, we should collide with this shape */
		/* if (this_->__IFSSTRUCT != NULL) collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_->__IFSSTRUCT); */
	       ~,
Rectangle2D => q~
		/* Modified Box code. */

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       struct pt kv = {0,0,0};
	       struct pt ov = {0,0,0};

	       struct pt t_orig = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */

	       struct pt delta;
	       struct pt tupv = {0,1,0};

		iv.x = $f(size,0);
		jv.y = $f(size,1);
		kv.z = 0.0;
		ov.x = -$f(size,0)/2; ov.y = -$f(size,1)/2; ov.z = 0.0;


	       /* get the transformed position of the Box, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_box_intersect(abottom,atop,awidth,t_orig,scale*$f(size,0),scale*$f(size,1),0.0)) return;

	       /* get transformed box edges and position */
	       transform(&ov,&ov,modelMatrix);
	       transform3x3(&iv,&iv,modelMatrix);
	       transform3x3(&jv,&jv,modelMatrix);
	       transform3x3(&kv,&kv,modelMatrix);


	       delta = box_disp(abottom,atop,astep,awidth,ov,iv,jv,kv);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);


		#ifdef COLLISIONVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_BOX: (%f %f %f) (%f %f %f)\n",
			  ov.x, ov.y, ov.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) kv=(%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  kv.x, kv.y, kv.z
			  );
		#endif

	       ~,
TriangleSet2D => q~
		/* yeah, yeah, we should collide with this shape */
		/* if (this_->__IFSSTRUCT != NULL) collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_->__IFSSTRUCT); */
	       ~,

Sphere => q~
	       struct pt t_orig; /*transformed origin*/
	       struct pt p_orig; /*projected transformed origin */
	       struct pt n_orig; /*normal(unit length) transformed origin */
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       GLdouble dist2;
	       struct pt delta = {0,0,0};
	       GLdouble radius;

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/

	       struct pt tupv = {0,1,0};

		/* are we initialized yet? */
		if (this_->__points==0) {
			return;
		}

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       radius = pow(det3x3(modelMatrix),1./3.) * $f(radius);

	       /* squared distance to center of sphere (on the y plane)*/
	       dist2 = t_orig.x * t_orig.x + t_orig.z * t_orig.z;

	       /* easy tests. clip as if sphere was a box */
	       /*clip with cylinder */
	       if(dist2 - (radius + awidth) * (radius +awidth) > 0) {
		   return;
	       }
	       /*clip with bottom plane */
	       if(t_orig.y + radius < abottom) {
		   return;
	       }
	       /*clip with top plane */
	       if(t_orig.y-radius > atop) {
		   return;
	       }

	       /* project onto (y x t_orig) plane */
	       p_orig.x = sqrt(dist2);
	       p_orig.y = t_orig.y;
	       p_orig.z = 0;
	       /* we need this to unproject rapidly */
	       /* n_orig is t_orig.y projected on the y plane, then normalized. */
	       n_orig.x = t_orig.x;
	       n_orig.y = 0.0;
	       n_orig.z = t_orig.z;
	       VECSCALE(n_orig,1.0/p_orig.x); /*equivalent to vecnormal(n_orig);, but faster */

	       /* 5 cases : sphere is over, over side, side, under and side, under (relative to y axis) */
	       /* these 5 cases correspond to the 5 vornoi regions of the cylinder */
	       if(p_orig.y > atop) {

		   if(p_orig.x < awidth) {
			#ifdef RENDERVERBOSE
		       printf(" /* over, we push down. */ \n");
			#endif

		       delta.y = (p_orig.y - radius) - (atop);
		   } else {
		       struct pt d2s;
		       GLdouble ratio;
		       #ifdef RENDERVERBOSE
			printf(" /* over side */ \n");
			#endif

		       /* distance vector from corner to center of sphere*/
		       d2s.x = p_orig.x - awidth;
		       d2s.y = p_orig.y - (atop);
		       d2s.z = 0;

		       ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);

		       if(ratio >= 0) {
			   /* no collision */
			   return;
		       }

		       /* distance vector from corner to surface of sphere, (do the math) */
		       VECSCALE(d2s, ratio );

		       /* unproject, this is the fastest way */
		       delta.y = d2s.y;
		       delta.x = d2s.x* n_orig.x;
		       delta.z = d2s.x* n_orig.z;
		   }
	       } else if(p_orig.y < abottom) {
		   if(p_orig.x < awidth) {
			#ifdef RENDERVERBOSE
		       printf(" /* under, we push up. */ \n");
			#endif

		       delta.y = (p_orig.y + radius) -abottom;
		   } else {
		       struct pt d2s;
		       GLdouble ratio;
			#ifdef RENDERVERBOSE
		       printf(" /* under side */ \n");
			#endif

		       /* distance vector from corner to center of sphere*/
		       d2s.x = p_orig.x - awidth;
		       d2s.y = p_orig.y - abottom;
		       d2s.z = 0;

		       ratio = 1- radius/sqrt(d2s.x * d2s.x + d2s.y * d2s.y);

		       if(ratio >= 0) {
			   /* no collision */
			   return;
		       }

		       /* distance vector from corner to surface of sphere, (do the math) */
		       VECSCALE(d2s, ratio );

		       /* unproject, this is the fastest way */
		       delta.y = d2s.y;
		       delta.x = d2s.x* n_orig.x;
		       delta.z = d2s.x* n_orig.z;
		   }

	       } else {
		   #ifdef RENDERVERBOSE
			printf(" /* side */ \n");
		   #endif

		   /* push to side */
		   delta.x = ((p_orig.x - radius)- awidth) * n_orig.x;
		   delta.z = ((p_orig.x - radius)- awidth) * n_orig.z;
	       }


	       transform3x3(&delta,&delta,upvecmat);
	       accumulate_disp(&CollisionInfo,delta);

		#ifdef COLLISIONVERBOSE
	       if((delta.x != 0. || delta.y != 0. || delta.z != 0.))
	           printf("COLLISION_SPH: (%f %f %f) (%f %f %f) (px=%f nx=%f nz=%f)\n",
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z,
			  p_orig.x, n_orig.x, n_orig.z
			  );
		#endif

	       ~,
Box => q~

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       struct pt kv = {0,0,0};
	       struct pt ov = {0,0,0};

	       struct pt t_orig = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */

	       struct pt delta;
	       struct pt tupv = {0,1,0};

		iv.x = $f(size,0);
		jv.y = $f(size,1);
		kv.z = $f(size,2);
		ov.x = -$f(size,0)/2; ov.y = -$f(size,1)/2; ov.z = -$f(size,2)/2;


	       /* get the transformed position of the Box, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_box_intersect(abottom,atop,awidth,t_orig,scale*$f(size,0),scale*$f(size,1),scale*$f(size,2))) return;

	       /* get transformed box edges and position */
	       transform(&ov,&ov,modelMatrix);
	       transform3x3(&iv,&iv,modelMatrix);
	       transform3x3(&jv,&jv,modelMatrix);
	       transform3x3(&kv,&kv,modelMatrix);


	       delta = box_disp(abottom,atop,astep,awidth,ov,iv,jv,kv);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);


		#ifdef COLLISIONVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_BOX: (%f %f %f) (%f %f %f)\n",
			  ov.x, ov.y, ov.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) kv=(%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  kv.x, kv.y, kv.z
			  );
		#endif


	       ~,

Cone => q~

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

		float h = $f(height)/2;
		float r = $f(bottomRadius);

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */
	       struct pt t_orig = {0,0,0};

	       struct pt delta;
	       struct pt tupv = {0,1,0};

	       iv.y = h; jv.y = -h;

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);

	       if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;

	       /* get transformed box edges and position */
	       transform(&iv,&iv,modelMatrix);
	       transform(&jv,&jv,modelMatrix);

	       delta = cone_disp(abottom,atop,astep,awidth,jv,iv,scale*r);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

		#ifdef COLLISIONVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_CON: (%f %f %f) (%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  scale*r
			  );
		#endif


	       ~,

Cylinder => q~

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

		float h = $f(height)/2;
		float r = $f(radius);

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */
	       struct pt t_orig = {0,0,0};

	       struct pt tupv = {0,1,0};
	       struct pt delta;


		iv.y = h;
		jv.y = -h;

	       /* get the transformed position of the Sphere, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;



	       /* get transformed box edges and position */
	       transform(&iv,&iv,modelMatrix);
	       transform(&jv,&jv,modelMatrix);


	       delta = cylinder_disp(abottom,atop,astep,awidth,jv,iv,scale*r);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

		#ifdef COLLISIONVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_CYL: (%f %f %f) (%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) bR=%f\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  scale*r
			  );
		#endif


	       ~,


ElevationGrid => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,
IndexedFaceSet => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,

IndexedTriangleFanSet => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,


IndexedTriangleSet => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,


IndexedTriangleStripSet => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,


TriangleFanSet => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,

TriangleStripSet => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,


TriangleSet => q~
		collideIndexedFaceSet ((struct X3D_IndexedFaceSet *) this_);
~,

Extrusion => q~


	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];

	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */
	       struct pt t_orig = {0,0,0};
	       static int refnum = 0;

	       struct pt tupv = {0,1,0};
	       struct pt delta = {0,0,0};

	       struct X3D_PolyRep pr;
	       prflags flags = 0;
	       int change = 0;

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

	       if(!$f(solid)) {
		   flags = flags | PR_DOUBLESIDED;
	       }
/*	       printf("_PolyRep = %d\n",this_->_intern);*/
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
	       scale = pow(det3x3(modelMatrix),1./3.);
/*	       if(!fast_ycylinder_cone_intersect(abottom,atop,awidth,t_orig,scale*h,scale*r)) return;*/

/*	       printf("ntri=%d\n",pr.ntri);
	       for(i = 0; i < pr.ntri; i++) {
		   printf("cindex[%d]=%d\n",i,pr.cindex[i]);
	       }*/
	       delta = polyrep_disp(abottom,atop,astep,awidth,pr,modelMatrix,flags);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

		#ifdef COLLISIONVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
/*		   printmatrix(modelMatrix);*/
		   fprintf(stderr,"COLLISION_EXT: ref%d (%f %f %f) (%f %f %f)\n",refnum++,
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );
	       }
		#endif

~,

Text => q~
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;
	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];

	       struct pt t_orig = {0,0,0};
	       static int refnum = 0;

		/*JAS - normals are always this way - helps because some
			normal calculations failed because of very small triangles
			which made collision calcs fail, which moved the Viewpoint...
			so, if there is no need to calculate normals..., why do it? */
	       struct pt tupv = {0,1,0};
	       struct pt delta = {0,0,-1};
	       struct X3D_PolyRep pr;
	       int change = 0;


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

	       delta = planar_polyrep_disp(abottom,atop,astep,awidth,pr,modelMatrix,PR_DOUBLESIDED,delta); /*delta used as zero*/

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);

		#ifdef COLLISIONVERBOSE 
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))  {
		   fprintf(stderr,"COLLISION_TXT: ref%d (%f %f %f) (%f %f %f)\n",refnum++,
			  t_orig.x, t_orig.y, t_orig.z,
			  delta.x, delta.y, delta.z
			  );

	       }
		#endif

~,

GeoElevationGrid => q~
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

	       if(!$f(solid)) {
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


	       delta = elevationgrid_disp(abottom,atop,awidth,astep,pr,$f(xDimension),$f(zDimension),xSpacing,zSpacing,
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
~,

);



1;
