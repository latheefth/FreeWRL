/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Lighting Component

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


void render_DirectionalLight (struct X3D_DirectionalLight *this_) {
	/* NOTE: This is called by the Group Children code
	 * at the correct point (in the beginning of the rendering
	 * of the children. We just turn the light on right now.
	 */

	if(((this_->on))) {
		int light = nextlight();
		if(light >= 0) {
			float vec[4];
			/* glEnable(light); */
			lightState(light-GL_LIGHT0,TRUE);
			vec[0] = -((this_->direction).c[0]);
			vec[1] = -((this_->direction).c[1]);
			vec[2] = -((this_->direction).c[2]);
			vec[3] = 0;
			glLightfv(light, GL_POSITION, vec);
			vec[0] = ((this_->color).c[0]) * (this_->intensity) /*cget*/;
			vec[1] = ((this_->color).c[1]) * (this_->intensity) /*cget*/;
			vec[2] = ((this_->color).c[2]) * (this_->intensity) /*cget*/;
			vec[3] = 1;
			glLightfv(light, GL_DIFFUSE, vec);
			glLightfv(light, GL_SPECULAR, vec);

			/* Aubrey Jaffer */
			vec[0] = ((this_->color).c[0]) * (this_->ambientIntensity) /*cget*/;
			vec[1] = ((this_->color).c[1]) * (this_->ambientIntensity) /*cget*/;
			vec[2] = ((this_->color).c[2]) * (this_->ambientIntensity) /*cget*/;

			glLightfv(light, GL_AMBIENT, vec);
		}
	}
}
