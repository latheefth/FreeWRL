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
