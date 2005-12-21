/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Shape Component

*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>
#ifdef AQUA
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "Structs.h"
#include "headers.h"
#include "installdir.h"

void render_LineProperties (struct X3D_LineProperties *this_) {
	GLint	factor;
	GLushort pat;

	if (this_->applied) {
		global_lineProperties=TRUE;
		if (this_->linewidthScaleFactor > 1.0) glLineWidth(this_->linewidthScaleFactor);
		if (this_->linetype > 0) {
			factor = 1;
			pat = 0xffff; /* can not support fancy line types - this is the default */
			switch (this_->linetype) {
				case 2: pat = 0xaaaa; break;
				case 3: pat = 0x4444; break;
				case 4: pat = 0xa4a4; break;
				case 5: pat = 0xaa44; break;
				case 6: pat = 0x0100; break;
				case 7: pat = 0x0100; break;
				case 10: pat = 0xaaaa; break;
				case 11: pat = 0x0170; break;
				case 12: pat = 0x0000; break;
				case 13: pat = 0x0000; break;
				default: {}
			}
			glLineStipple (factor,pat);
			glEnable(GL_LINE_STIPPLE);
		}
	}

}

void render_FillProperties (struct X3D_FillProperties *this_) {
		GLubyte halftone[] = {
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55};

		global_fillProperties=TRUE;

		if (!this_->filled) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (this_->hatched) {
			glColor3f (this_->hatchColor.c[0], this_->hatchColor.c[1],this_->hatchColor.c[2]);
			glPolygonStipple(halftone);			
			glEnable (GL_POLYGON_STIPPLE);
		}
}

void render_Material (struct X3D_Material *this_) {
		int i;
		float dcol[4];
		float ecol[4];
		float scol[4];
		float shin;
		float amb;
		float trans;

#ifndef X3DMATERIALPROPERTY
		/* We have to keep track of whether to reset diffuseColor if using
		   textures; no texture or greyscale, we use the diffuseColor, if
		   RGB we set diffuseColor to be grey */
		if (last_texture_depth >1) {
			dcol[0]=0.8, dcol[1]=0.8, dcol[2]=0.8;
		} else {
#endif

			for (i=0; i<3;i++){ dcol[i] = this_->diffuseColor.c[i]; }
#ifndef X3DMATERIALPROPERTY
		}
#endif

		/* set the transparency here for the material */
		trans = 1.0 - this_->transparency;
		if (trans<0.0) trans = 0.0;
		if (trans>=0.99) trans = 0.99;

		/* and, record that we have a transparency here */
		/* and record transparency value, in case we have an
		   indexedfaceset with colour node */
		if (trans <=0.99) {
			have_transparency++;
			if ((this_->_renderFlags & VF_Blend) != VF_Blend)
				update_renderFlag(this_,VF_Blend);
			last_transparency=trans;
		}

		dcol[3] = trans;

		do_glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dcol);

		amb = this_->ambientIntensity;
		for(i=0; i<3; i++) {
                       /* to make this render like Xj3D, make ambient 0 with only headlight */
                        /* dcol[i] *= amb; */
                        dcol[i] = 0.0;
		}
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, dcol);

		for (i=0; i<3;i++){ scol[i] = this_->specularColor.c[i]; }
		scol[3] = trans;
		/* scol[3] = 1.0;*/
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, scol);

		for (i=0; i<3;i++){ ecol[i] = this_->emissiveColor.c[i]; }
		ecol[3] = trans;
		/* ecol[3] = 1.0;*/

		do_glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ecol);
		glColor3f(ecol[0],ecol[1],ecol[2]);

		shin = 128.0* this_->shininess;
		do_shininess(shin);
}


void child_Shape (struct X3D_Shape *this_) {
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
       	        if((this_->appearance)) {
                        render_node((this_->appearance));
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
}


void child_Appearance (struct X3D_Appearance *this_) {
                last_texture_depth = 0;
                last_transparency = 1.0;

        /* printf ("in Appearance, this %d, nodeType %d\n",this_, this_->_nodeType);
         printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
         render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

                if((this_->material)) {
                        render_node((this_->material));
                } else {
                        /* no material, so just colour the following shape */
                        /* Spec says to disable lighting and set coloUr to 1,1,1 */
                        glDisable (GL_LIGHTING);
                        glColor3f(1.0,1.0,1.0);
                        lightingOn = FALSE;
                }

                if ((this_->fillProperties)) {
                        render_node((this_->fillProperties));
                }

                /* set line widths - if we have line a lineProperties node */
                if ((this_->lineProperties)) {
                        render_node((this_->lineProperties));
                }

                if((this_->texture)) {
                        /* we have to do a glPush, then restore, later */
                        have_texture=TRUE;
                        glPushAttrib(GL_ENABLE_BIT);

                        /* is there a TextureTransform? if no texture, fugutaboutit */
                        this_textureTransform = this_->textureTransform;

                        /* now, render the texture */
                        render_node((this_->texture));
                }
}
